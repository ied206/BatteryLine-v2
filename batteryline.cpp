#include "var.h"

#include "batteryline.h"
#include "ui_batteryline.h"
#include "settingdialog.h"
#include "systemhelper.h"

#include <QTimer>
#include <QDebug>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>


BatteryLine::BatteryLine(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BatteryLine)
{
    ui->setupUi(this);

    // Layered Window + Always On Top
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

#ifdef Q_OS_WIN
    // Give WS_EX_NOACTIVE property
    hWnd = (HWND) this->winId();
    LONG dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    dwExStyle = dwExStyle | WS_EX_NOACTIVATE;
    SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);
#endif

    // Program Info
    QCoreApplication::setOrganizationName(BL_ORG_NAME);
    QCoreApplication::setOrganizationDomain(BL_ORG_DOMAIN);
    QCoreApplication::setApplicationName(BL_APP_NAME);

    // Init Member Classes
    m_powerStat = new PowerStatus();
#ifdef Q_OS_WIN
    m_powerNotify = new PowerNotify(hWnd);
#endif
#ifdef Q_OS_LINUX
    m_powerNotify = new PowerNotify();
#endif
    // Connect Signal with m_powerNotify
    connect(m_powerNotify, &PowerNotify::RedrawSignal, this, &BatteryLine::DrawLine);

    // Init Member Variables - Setting and Context Menu
    m_setting = nullptr;

    trayIconMenu = nullptr;
    trayIcon = nullptr;

    printBannerAct = nullptr;
    printHelpAct = nullptr;
    openHomepageAct = nullptr;
    openLicenseAct = nullptr;
    openSettingAct = nullptr;
    printPowerInfoAct = nullptr;
    exitAct = nullptr;

    // Connect signal with screen change
    connect(QApplication::desktop(), &QDesktopWidget::primaryScreenChanged, this, &BatteryLine::PrimaryScreenChanged);
    connect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, this, &BatteryLine::ScreenCountChanged);
    connect(QApplication::desktop(), &QDesktopWidget::resized, this, &BatteryLine::ScreenResized);
    connect(QApplication::desktop(), &QDesktopWidget::workAreaResized, this, &BatteryLine::ScreenWorkAreaResized);

    // Tray Icon
    CreateTrayIcon();

    // Load Settings from INI
    m_setting = new QSettings(QSettings::IniFormat, QSettings::UserScope, BL_ORG_NAME, BL_APP_NAME);
    memset(static_cast<void*>(&m_option), 0, sizeof(BL_OPTION));
    ReadSettings();

    // Set Windows Size and Position, Color
    setAutoFillBackground(true);
    DrawLine();
}

BatteryLine::~BatteryLine()
{
    // Connect signal with screen change
    disconnect(QApplication::desktop(), &QDesktopWidget::primaryScreenChanged, this, &BatteryLine::PrimaryScreenChanged);
    disconnect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, this, &BatteryLine::ScreenCountChanged);
    disconnect(QApplication::desktop(), &QDesktopWidget::resized, this, &BatteryLine::ScreenResized);
    disconnect(QApplication::desktop(), &QDesktopWidget::workAreaResized, this, &BatteryLine::ScreenWorkAreaResized);

    // Disconnect Signal with m_powerNotify
    disconnect(m_powerNotify, &PowerNotify::RedrawSignal, this, &BatteryLine::DrawLine);

    delete ui;
    delete m_powerStat;

    delete m_powerNotify;

    delete m_setting;

    delete trayIconMenu;
    delete trayIcon;

    delete printBannerAct;
    delete printHelpAct;
    delete openHomepageAct;
    delete openLicenseAct;
    delete openSettingAct;
    delete printPowerInfoAct;
    delete exitAct;

    ui = nullptr;
    m_powerStat = nullptr;
    m_powerNotify = nullptr;

    m_setting = nullptr;

    trayIconMenu = nullptr;
    trayIcon = nullptr;

    printBannerAct = nullptr;
    printHelpAct = nullptr;
    openHomepageAct = nullptr;
    openLicenseAct = nullptr;
    openSettingAct = nullptr;
    printPowerInfoAct = nullptr;
    exitAct = nullptr;
}

void BatteryLine::DrawLine()
{
    m_powerStat->Update();
    SetColor();
    SetWindowSizePos();
}

// Must update m_batStat first
void BatteryLine::SetWindowSizePos()
{
    QRect screenWorkRect; // availableGeometry - Resolution excluding taskbar
    QRect screenFullRect; // screenGeometry - Full resoultion
    QRect appRect, trayRect;
    SettingPosition taskbar;
    int targetScreen = 0;

    if (m_option.mainMonitor == true) // Main Monitor
        targetScreen = -1;
    else // Custom Monitor
        targetScreen = m_option.customMonitor - 1;

    screenWorkRect = QApplication::desktop()->availableGeometry(targetScreen);
    screenFullRect = QApplication::desktop()->screenGeometry(targetScreen);

    // Calculate where taskbar is, using screenWorkRect and screenFullRect's relation.
    if (screenFullRect.top() != screenWorkRect.top())
    { // TaskBar is on TOP
        taskbar = SettingPosition::Top;
        trayRect.setLeft(screenFullRect.left());
        trayRect.setTop(screenFullRect.top());
        trayRect.setWidth(screenFullRect.width());
        trayRect.setHeight(screenWorkRect.top() - screenFullRect.top());
    }
    else if (screenFullRect.bottom() != screenWorkRect.bottom())
    { // TaskBar is on BOTTOM
        taskbar = SettingPosition::Bottom;
        trayRect.setLeft(screenFullRect.left());
        trayRect.setTop(screenWorkRect.bottom());
        trayRect.setWidth(screenFullRect.width());
        trayRect.setHeight(screenFullRect.bottom() - screenWorkRect.bottom());
    }
    else if (screenFullRect.left() != screenWorkRect.left())
    { // TaskBar is on LEFT
        taskbar = SettingPosition::Left;
        trayRect.setLeft(screenFullRect.left());
        trayRect.setTop(screenFullRect.top());
        trayRect.setWidth(screenWorkRect.left() - screenFullRect.left());
        trayRect.setHeight(screenFullRect.height());
    }
    else if (screenFullRect.right() != screenWorkRect.right())
    { // TaskBar is on RIGHT
        taskbar = SettingPosition::Right;
        trayRect.setLeft(screenWorkRect.right());
        trayRect.setTop(screenFullRect.top());
        trayRect.setWidth(screenFullRect.width());
        trayRect.setHeight(screenFullRect.height());
    }

    int appLeft = 0;
    int appTop = 0;
    int appWidth = 0;
    int appHeight = 0;

    switch (m_option.position)
    {
    case SettingPosition::Top:
        if (m_powerStat->m_BatteryFull) // Not Charging, because battery is full
            appWidth = screenFullRect.width();
        else
            appWidth = (screenFullRect.width() * m_powerStat->m_BatteryLevel) / 100;

        // Evade taskbar or not
        if (m_option.taskbar == static_cast<int>(SettingTaskBar::Evade)
                  && taskbar == SettingPosition::Top) // TaskBar is on TOP - conflict
            appTop = trayRect.bottom();
        else // TaskBar is on BOTTOM | LEFT | RIGHT
            appTop = screenFullRect.top();

        appLeft = screenFullRect.left();
        appHeight = m_option.height;
        break;

    case SettingPosition::Bottom:
        if (m_powerStat->m_BatteryFull) // Not Charging, because battery is full
            appWidth = screenFullRect.width();
        else
            appWidth = (screenFullRect.width() * m_powerStat->m_BatteryLevel) / 100;

        // Evade taskbar or not
        if (m_option.taskbar == static_cast<int>(SettingTaskBar::Evade)
                  && taskbar == SettingPosition::Bottom) // TaskBar is on BOTTOM - conflict
            appTop = trayRect.top() - m_option.height;
        else // TaskBar is on TOP | LEFT | RIGHT
            appTop = screenFullRect.bottom() - m_option.height;

        appLeft = screenFullRect.left();
        appHeight = m_option.height;
        break;

    case SettingPosition::Left:
        if (m_powerStat->m_BatteryFull) // Not Charging, because battery is full
            appHeight = screenFullRect.height();
        else
            appHeight = (screenFullRect.height() * m_powerStat->m_BatteryLevel) / 100;

        // Evade taskbar or not
        if (m_option.taskbar == static_cast<int>(SettingTaskBar::Evade)
                  && taskbar == SettingPosition::Left) // TaskBar is on LEFT - conflict
            appLeft = trayRect.left() + trayRect.width();
        else // TaskBar is on TOP | BOTTOM | RIGHT
            appLeft = screenFullRect.left();

        appTop = screenFullRect.top();
        appWidth = m_option.height;
        break;

    case SettingPosition::Right:
        if (m_powerStat->m_BatteryFull) // Not Charging, because battery is full
            appHeight = screenFullRect.height();
        else
            appHeight = (screenFullRect.height() * m_powerStat->m_BatteryLevel) / 100;

        // Evade taskbar or not
        if (m_option.taskbar == static_cast<int>(SettingTaskBar::Evade)
                  && taskbar == SettingPosition::Right) // TaskBar is on Right - conflict
            appLeft = trayRect.left() - m_option.height;
        else // TaskBar is on TOP | BOTTOM | LEFT
            appLeft = screenFullRect.left() + screenFullRect.width() - m_option.height;

        appTop = screenFullRect.top();
        appWidth = m_option.height;
        break;
    }

    appRect.setLeft(appLeft);
    appRect.setTop(appTop);
    appRect.setWidth(appWidth);
    appRect.setHeight(appHeight);

    setGeometry(appRect);
    updateGeometry();

    qDebug() << QString("[Monitor]");
    qDebug() << QString("Displaying on monitor %1").arg(m_option.customMonitor);
    qDebug() << QString("Base Coordinate        : (%1, %2)").arg(screenFullRect.left()).arg(screenFullRect.top());
    qDebug() << QString("Screen Resolution      : (%1, %2)").arg(screenFullRect.width()).arg(screenFullRect.height());
    qDebug() << QString("Taskbar Coordinate     : (%1, %2)").arg(trayRect.left()).arg(trayRect.top());
    qDebug() << QString("Taskbar Resolution     : (%1, %2)").arg(trayRect.width()).arg(trayRect.height());
    qDebug() << QString("BatteryLine Coordinate : (%1, %2)").arg(appRect.left()).arg(appRect.top());
    qDebug() << QString("BatteryLine Resolution : (%1, %2)").arg(appRect.width()).arg(appRect.height());
    qDebug() << "";
}

// Must update m_batStat first
void BatteryLine::SetColor()
{
    QPalette palette;
    QColor color;
    color.setAlpha(255);
    setWindowOpacity(static_cast<qreal>(m_option.transparency) / 255);

    if (m_powerStat->m_ACLineStatus == false) // Not Charging, running on battery
    {
        qDebug() << "Not Charging";
        color.setRed(0);
        color.setGreen(255);
        color.setBlue(0);
    }
    else if (m_powerStat->m_BatteryFull == true) // Not Charging, because battery is full
    { // Even though BatteryLifePercent is not 100, consider it as 100
        qDebug() << "Battery Full";
        color.setRed(0);
        color.setGreen(162);
        color.setBlue(232);
    }
    else if (m_powerStat->m_BatteryCharging == true)  // Charging, and show charge color option set
    {
        color.setRed(0);
        color.setGreen(200);
        color.setBlue(255);
    }
    else
        SystemHelper::SystemError("[General] Invalid battery status data");

    palette.setColor(QPalette::Background, color);
    this->setPalette(palette);
}

void BatteryLine::PrimaryScreenChanged()
{
    DrawLine();
}

void BatteryLine::ScreenCountChanged(int newCount)
{
    (void) newCount;
    DrawLine();
}

void BatteryLine::ScreenResized(int screen)
{
    (void) screen;
    DrawLine();
}

void BatteryLine::ScreenWorkAreaResized(int screen)
{
    (void) screen;
    DrawLine();
}

// http://doc.qt.io/qt-5/qtwidgets-mainwindows-menus-example.html
void BatteryLine::CreateTrayIcon()
{
    // Init
    trayIconMenu = new QMenu(this);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

    // Icon
    QIcon icon = QIcon(":/images/Cycle.png");
    trayIcon->setIcon(icon);
    setWindowIcon(icon);

    // Context Menu
    printBannerAct = new QAction(tr("A&bout"), this);
    printBannerAct->setIcon(icon);
    connect(printBannerAct, &QAction::triggered, this, &BatteryLine::TrayMenuPrintBanner);
    trayIconMenu->addAction(printBannerAct);

    printHelpAct = new QAction(tr("&Help"), this);
    connect(printHelpAct, &QAction::triggered, this, &BatteryLine::TrayMenuPrintHelp);
    trayIconMenu->addAction(printHelpAct);
    trayIconMenu->addSeparator();

    openHomepageAct = new QAction(tr("&Homepage"), this);
    connect(openHomepageAct, &QAction::triggered, this, &BatteryLine::TrayMenuHomepage);
    trayIconMenu->addAction(openHomepageAct);

    openLicenseAct = new QAction(tr("&License"), this);
    connect(openLicenseAct, &QAction::triggered, this, &BatteryLine::TrayMenuLicense);
    trayIconMenu->addAction(openLicenseAct);
    trayIconMenu->addSeparator();

    openSettingAct = new QAction(tr("&Setting"), this);
    connect(openSettingAct, &QAction::triggered, this, &BatteryLine::TrayMenuSetting);
    trayIconMenu->addAction(openSettingAct);

    printPowerInfoAct = new QAction(tr("&Power Info"), this);
    connect(printPowerInfoAct, &QAction::triggered, this, &BatteryLine::TrayMenuPowerInfo);
    trayIconMenu->addAction(printPowerInfoAct);
    trayIconMenu->addSeparator();

    exitAct = new QAction(tr("E&xit"), this);
    connect(exitAct, &QAction::triggered, this, &BatteryLine::TrayMenuExit);
    trayIconMenu->addAction(exitAct);

    // Event Handler
    connect(trayIcon, &QSystemTrayIcon::activated, this, &BatteryLine::TrayIconClicked);

    // Show
    trayIcon->show();
}

void BatteryLine::TrayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::Trigger: // SingleClick
        TrayMenuPrintBanner();
        break;
    case QSystemTrayIcon::Context: // RightClick
    case QSystemTrayIcon::DoubleClick: // DoubleClick
    case QSystemTrayIcon::MiddleClick: // MiddleClick
    case QSystemTrayIcon::Unknown:
    default:
        break;
    }
}

void BatteryLine::TrayMenuExit()
{
    QCoreApplication::exit(0);
}

void BatteryLine::TrayMenuPrintBanner()
{
    QString msgStr, webBinary, webSource;

    webBinary = BL_WEB_BINARY;
    webSource = BL_WEB_SOURCE;
    msgStr = QString("Joveler's BatteryLine v%1.%2 (%3bit)\n"
                  "Show battery status as line in screen.\n\n"
                  "[Binary] %4\n"
                  "[Source] %5\n\n"
                  "Build %6%7%8")
            .arg(BL_MAJOR_VER)
            .arg(BL_MINOR_VER)
            .arg(SystemHelper::WhatBitOS())
            .arg(webBinary)
            .arg(webSource)
            .arg(SystemHelper::CompileYear())
            .arg(SystemHelper::CompileMonth())
            .arg(SystemHelper::CompileDay());

    QMessageBox msgBox;
    msgBox.setText(BL_APP_NAME);
    msgBox.setInformativeText(msgStr);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    // This method crashes Linux Mint's Cinnamon Desktop
    //QMessageBox::information(this, tr("BatteryLine"), msg);
}

void BatteryLine::TrayMenuPrintHelp()
{
    QString msgStr, webBinary, webSource;

    webBinary = BL_WEB_BINARY;
    webSource = BL_WEB_SOURCE;
    msgStr = QString("[BatteryLine Help Message]\n"
                  "Show battery status as line in screen.\n\n"
                  "[Command Line Option]\n"
                 "-q : Launch this program without notification.\n"
                 "-h : Print this help message and exit.\n\n"
                 "[Setting]\n"
                 "You can edit BatteryLine's setting in BatteryLine.ini.");

    QMessageBox msgBox;
    msgBox.setText(BL_APP_NAME);
    msgBox.setInformativeText(msgStr);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    // This method crashes Linux Mint's Cinnamon Desktop
    //QMessageBox::information(this, tr("BatteryLine"), msg);
}

void BatteryLine::TrayMenuHomepage()
{
    // Open project homepage
    QDesktopServices::openUrl(QUrl(BL_WEB_BINARY));
}

void BatteryLine::TrayMenuLicense()
{
    // Open GitHub repository's LICENSE page
    QDesktopServices::openUrl(QUrl(BL_WEB_LICENSE));
}

void BatteryLine::TrayMenuSetting()
{
    SettingDialog dialog(this);
    connect(&dialog, SIGNAL(SignalGeneral(SettingGeneralKey, QVariant)), this, SLOT(SettingGeneral(SettingGeneralKey, QVariant)));
    connect(&dialog, SIGNAL(SignalBasicColor(SettingBasicColorKey, QVariant)), this, SLOT(SettingBasicColor(SettingBasicColorKey,QVariant)));
    connect(&dialog, SIGNAL(SignalCustomColor(SettingCustomColorKey, int, QVariant)), this, SLOT(SettingCustomColor(SettingCustomColorKey, int, QVariant)));
    dialog.exec();
    disconnect(&dialog, SIGNAL(SignalGeneral(SettingGeneralKey, QVariant)), this, SLOT(SettingGeneral(SettingGeneralKey, QVariant)));
    disconnect(&dialog, SIGNAL(SignalBasicColor(SettingBasicColorKey, QVariant)), this, SLOT(SettingBasicColor(SettingBasicColorKey,QVariant)));
    disconnect(&dialog, SIGNAL(SignalCustomColor(SettingCustomColorKey, int, QVariant)), this, SLOT(SettingCustomColor(SettingCustomColorKey, int, QVariant)));
}

void BatteryLine::TrayMenuPowerInfo()
{
    m_powerStat->Update();

    QString msgAcPower, msgCharge, msgFull;
    if (m_powerStat->m_ACLineStatus == true)
        msgAcPower = tr("AC");
    else
        msgAcPower = tr("Battery");

    if (m_powerStat->m_BatteryFull == true)
        msgCharge = tr("Full");
    else if (m_powerStat->m_BatteryCharging == true)
        msgCharge = tr("Charging");
    else if (m_powerStat->m_ACLineStatus == false)
        msgCharge = tr("Using Battery");
    else
        SystemHelper::SystemError("[General] Invalid battery status data");

    msgFull = QString("Power Source : %1\n"
                     "Battery Status : %2\n"
                     "Battery Percent : %3%\n")
            .arg(msgAcPower)
            .arg(msgCharge)
            .arg(m_powerStat->m_BatteryLevel);

    QMessageBox msgBox;
    msgBox.setText(tr("Power Info"));
    msgBox.setInformativeText(msgFull);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

void BatteryLine::SettingGeneral(SettingGeneralKey key, QVariant entry)
{
    switch (key)
    {
    case SettingGeneralKey::Height:
        m_option.height = entry.toUInt();
        break;
    case SettingGeneralKey::Position:
        m_option.position = entry.toUInt();
        break;
    case SettingGeneralKey::Transparency:
        m_option.transparency = entry.toUInt();
        break;
    case SettingGeneralKey::ShowCharge:
        m_option.showCharge = entry.toBool();
        break;
    case SettingGeneralKey::TaskBar:
        m_option.taskbar = entry.toUInt();
        break;
    case SettingGeneralKey::Monitor:
        if (entry.toUInt() == static_cast<int>(SettingMonitor::Primary))
            m_option.mainMonitor = true;
        else
            m_option.customMonitor = entry.toUInt();
        break;
    }

    DrawLine();
    WriteSettings();

    qDebug() << static_cast<int>(key);
    qDebug() << entry;
}

void BatteryLine::SettingBasicColor(SettingBasicColorKey key, QVariant entry)
{
    m_setting->beginGroup("basicColor");

    switch (key)
    {
    case SettingBasicColorKey::DefaultColor:
        m_option.defaultColor = entry.toInt();
        m_setting->setValue("defaultcolor", entry);
        break;
    case SettingBasicColorKey::ChargeColor:
        m_option.chargeColor = entry.toInt();
        m_setting->setValue("chargecolor", entry);
        break;
    case SettingBasicColorKey::FullColor:
        m_option.fullColor = entry.toInt();
        m_setting->setValue("fullcolor", entry);
        break;
    }

    m_setting->endGroup();
    DrawLine();

    qDebug() << static_cast<int>(key);
    qDebug() << entry;
}

void BatteryLine::SettingCustomColor(SettingCustomColorKey key, int index, QVariant entry)
{
    m_setting->beginGroup("customColor");

    switch (key)
    {
    case SettingCustomColorKey::LowEdge:
        m_option.lowEdge[index] = entry.toInt();
        m_setting->setValue(QString("lowedge%1").arg(index), entry);
        break;
    case SettingCustomColorKey::HighEdge:
        m_option.highEdge[index] = entry.toInt();
        m_setting->setValue(QString("highedge%1").arg(index), entry);
        break;
    case SettingCustomColorKey::Color:
        m_option.customColor[index] = entry.toInt();
        m_setting->setValue(QString("color%1").arg(index), entry);
        break;
    }

    m_setting->endGroup();
    DrawLine();

    qDebug() << static_cast<int>(key);
    qDebug() << entry;
}

void BatteryLine::ReadSettings()
{
    qDebug() << m_setting->fileName();

    // Default Value
    m_setting->beginGroup("General");
    m_option.height = m_setting->value("height", 5).toUInt();
    m_option.position = m_setting->value("position", static_cast<int>(SettingPosition::Top)).toUInt();
    m_option.transparency = m_setting->value("transparency", 196).toUInt();
    m_option.showCharge = m_setting->value("showcharge", true).toBool();
    m_option.taskbar = m_setting->value("taskbar", static_cast<int>(SettingTaskBar::Evade)).toUInt();
    uint monitor = m_setting->value("monitor", static_cast<int>(SettingMonitor::Primary)).toUInt();
    if (monitor == static_cast<int>(SettingMonitor::Primary))
    {
        m_option.mainMonitor = true;
        m_option.customMonitor = 0;
    }
    else
    {
        m_option.mainMonitor = false;
        m_option.customMonitor = monitor;
    }
    m_setting->endGroup();

    m_setting->beginGroup("BasicColor");
    m_option.defaultColor = SystemHelper::RGB_QStringToQColor(m_setting->value("defaultcolor", SystemHelper::RGB_QColorToQString(QColor(0, 255, 0))).toString());
    m_option.chargeColor = SystemHelper::RGB_QStringToQColor(m_setting->value("chargecolor", SystemHelper::RGB_QColorToQString(QColor(0, 200, 255))).toString());
    m_option.fullColor = SystemHelper::RGB_QStringToQColor(m_setting->value("fullcolor", SystemHelper::RGB_QColorToQString(QColor(0, 162, 232))).toString());
    m_setting->endGroup();

    m_setting->beginGroup("CustomColor");
    m_option.lowEdge[0] = m_setting->value("lowedge1", 0).toUInt();
    m_option.highEdge[0] = m_setting->value("highedge1", 20).toUInt();
    m_option.customColor[0] = SystemHelper::RGB_QStringToQColor(m_setting->value("customcolor1", SystemHelper::RGB_QColorToQString(QColor(237, 28, 36))).toString());
    m_option.lowEdge[1] = m_setting->value("lowedge2", 20).toUInt();
    m_option.highEdge[1] = m_setting->value("highedge2", 50).toUInt();
    m_option.customColor[1] = SystemHelper::RGB_QStringToQColor(m_setting->value("customcolor2", SystemHelper::RGB_QColorToQString(QColor(255, 140, 15))).toString());
    m_option.customColorCount = BL_COLOR_LEVEL;
    for (uint i = 3; i <= BL_COLOR_LEVEL; i++)
    {
        m_option.lowEdge[i-1] = m_setting->value(QString("lowedge%1").arg(i), 0).toUInt();
        m_option.highEdge[i-1] = m_setting->value(QString("highedge%1").arg(i), 0).toUInt();
        m_option.customColor[i-1] = SystemHelper::RGB_QStringToQColor(m_setting->value(QString("customcolor%1").arg(i), SystemHelper::RGB_QColorToQString(QColor(0, 0, 0))).toString());
        if (m_option.lowEdge[i-1] == 0 && m_option.highEdge[i-1] == 0 && m_option.customColor[i-1] == QColor(0, 0, 0)) // No value
        {
            m_option.customColorCount = i-1;
            break;
        }
    }
    m_setting->endGroup();
}

void BatteryLine::WriteSettings(bool defaultValue)
{
    // Default Value
    if (defaultValue)
    {
        m_setting->beginGroup("General");
        m_setting->setValue("height", 5);
        m_setting->setValue("position", static_cast<int>(SettingPosition::Top));
        m_setting->setValue("transparency", 196);
        m_setting->setValue("showcharge", true);
        m_setting->setValue("taskbar", static_cast<int>(SettingTaskBar::Evade));
        m_setting->setValue("monitor", static_cast<int>(SettingMonitor::Primary));
        m_setting->endGroup();

        m_setting->beginGroup("BasicColor");
        m_setting->setValue("defaultcolor", SystemHelper::RGB_QColorToQString(QColor(0, 255, 0)));
        m_setting->setValue("chargecolor", SystemHelper::RGB_QColorToQString(QColor(0, 200, 255)));
        m_setting->setValue("fullcolor", SystemHelper::RGB_QColorToQString(QColor(0, 162, 232)));
        m_setting->endGroup();

        m_setting->beginGroup("CustomColor");
        m_setting->setValue("lowedge1", 0);
        m_setting->setValue("highedge1", 20);
        m_setting->setValue("customcolor1", SystemHelper::RGB_QColorToQString(QColor(237, 28, 36)));
        m_setting->setValue("lowedge2", 20);
        m_setting->setValue("highedge2", 50);
        m_setting->setValue("customcolor2", SystemHelper::RGB_QColorToQString(QColor(255, 140, 15)));
        m_setting->endGroup();
    }
    else
    {
        m_setting->beginGroup("General");
        m_setting->setValue("height", m_option.height);
        m_setting->setValue("position", m_option.position);
        m_setting->setValue("transparency", m_option.transparency);
        m_setting->setValue("showcharge", m_option.showCharge);
        m_setting->setValue("taskbar", m_option.taskbar);
        if (m_option.mainMonitor == true)
            m_setting->setValue("monitor", 0);
        else
            m_setting->setValue("monitor", m_option.customMonitor);
        m_setting->endGroup();

        m_setting->beginGroup("BasicColor");
        m_setting->setValue("defaultcolor", SystemHelper::RGB_QColorToQString(m_option.defaultColor));
        m_setting->setValue("chargecolor", SystemHelper::RGB_QColorToQString(m_option.chargeColor));
        m_setting->setValue("fullcolor", SystemHelper::RGB_QColorToQString(m_option.fullColor));
        m_setting->endGroup();

        m_setting->beginGroup("CustomColor");
        for (uint i = 1; i <= m_option.customColorCount; i++)
        {
            m_setting->setValue(QString("lowedge%1").arg(i), m_option.lowEdge[i]);
            m_setting->setValue(QString("highedge%1").arg(i), m_option.highEdge[i]);
            m_setting->setValue(QString("customcolor%1").arg(i), SystemHelper::RGB_QColorToQString(m_option.customColor[i]));
        }
        m_setting->endGroup();
    }
}

#ifdef Q_OS_WIN
// http://doc.qt.io/qt-5/qwidget.html#nativeEvent
bool BatteryLine::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    if (eventType == "windows_generic_MSG")
    {
        MSG *msg = static_cast<MSG*>(message);

        switch (msg->message)
        {
        case WM_POWERBROADCAST: // Power source changed, battery level dropped
            qDebug() << "WM_POWERBROADCAST";
            DrawLine();
            break;
        case WM_DISPLAYCHANGE: // Monitor is attached or detached, Screen resolution changed, etc. Check for HMONITOR's validity.
            qDebug() << "WM_DISPLAYCHANGE";
            DrawLine();
            break;
        default:
            break;
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif
