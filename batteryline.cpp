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


BatteryLine::BatteryLine(bool quiet, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BatteryLine)
{
    ui->setupUi(this);

    // Layered Window + Always On Top
    // In Cinnamon/XOrg, window which has Qt::X11BypassWindowManagerHint cannot be parent of other window.
    setEnabled(false);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

#ifdef Q_OS_WIN
    // Give WS_EX_NOACTIVE property
    hWnd = (HWND) this->winId();
    LONG dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    dwExStyle |= WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);
#endif

    // Init Member Classes
    powerStat = new PowerStatus();
#ifdef Q_OS_WIN
    m_powerNotify = new PowerNotify(hWnd);
#endif
#ifdef Q_OS_LINUX
    powerNotify = new PowerNotify();
#endif
    // Connect Signal with m_powerNotify
    connect(powerNotify, &PowerNotify::RedrawSignal, this, &BatteryLine::DrawLine);

    // Init Member Variables - Setting and Context Menu
    setting = nullptr;

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
    setting = new QSettings(QSettings::IniFormat, QSettings::UserScope, BL_ORG_NAME, BL_APP_NAME);
    memset(static_cast<void*>(&option), 0, sizeof(BL_OPTION));
    ReadSettings();

    this->muteNotifcation = quiet;
    if (quiet == false)
    {
        // Popup
    }

    // Set Window Size and Position, Colorr
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
    disconnect(powerNotify, &PowerNotify::RedrawSignal, this, &BatteryLine::DrawLine);

    delete ui;
    delete powerStat;

    delete powerNotify;

    delete setting;

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
    powerStat = nullptr;
    powerNotify = nullptr;

    setting = nullptr;

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
    powerStat->Update();
    if (powerStat->BatteryExist == false)
    {
        if (muteNotifcation)
        {
            if (SystemHelper::setEventLoopRunning())
                exit(1);
            else
                QCoreApplication::exit(1);
        }
        else
            SystemHelper::SystemError(tr("There is no battery in this system.\nPlease attach battery and run again."));
    }
    SetColor();
    SetWindowSizePos();
}

// Must update m_batStat first
void BatteryLine::SetWindowSizePos()
{
    QRect screenWorkRect; // availableGeometry - Resolution excluding taskbar
    QRect screenFullRect; // screenGeometry - Full resoultion
    QRect appRect;
    int targetScreen = 0;

    if (option.mainMonitor == true) // Main Monitor
        targetScreen = -1;
    else // Custom Monitor
        targetScreen = option.customMonitor;

    screenWorkRect = QApplication::desktop()->availableGeometry(targetScreen);
    screenFullRect = QApplication::desktop()->screenGeometry(targetScreen);

    int appLeft = 0;
    int appTop = 0;
    int appWidth = 0;
    int appHeight = 0;

    switch (option.position)
    {
    case static_cast<int>(SettingPosition::Top):
        if (powerStat->BatteryFull) // Not Charging, because battery is full
            appWidth = screenWorkRect.width();
        else
            appWidth = (screenWorkRect.width() * powerStat->BatteryLevel) / 100;
        if (option.align == static_cast<int>(SettingAlign::LeftTop))
            appLeft = screenWorkRect.left();
        else // static_cast<int>(SettingAlign::RightBottom)
            appLeft = screenWorkRect.left() + screenWorkRect.width() - appWidth;
        appTop = screenWorkRect.top();
        appHeight = option.height;
        break;

    case static_cast<int>(SettingPosition::Bottom):
        if (powerStat->BatteryFull) // Not Charging, because battery is full
            appWidth = screenWorkRect.width();
        else
            appWidth = (screenWorkRect.width() * powerStat->BatteryLevel) / 100;
        if (option.align == static_cast<int>(SettingAlign::LeftTop))
            appLeft = screenWorkRect.left();
        else // static_cast<int>(SettingAlign::RightBottom)
            appLeft = screenWorkRect.left() + screenWorkRect.width() - appWidth;
        appTop = screenWorkRect.top() + screenWorkRect.height() - option.height;
        appHeight = option.height;
        break;

    case static_cast<int>(SettingPosition::Left):
        if (powerStat->BatteryFull) // Not Charging, because battery is full
            appHeight = screenWorkRect.height();
        else
            appHeight = (screenWorkRect.height() * powerStat->BatteryLevel) / 100;
        if (option.align == static_cast<int>(SettingAlign::LeftTop))
            appTop = screenWorkRect.top();
        else // static_cast<int>(SettingAlign::RightBottom)
            appTop = screenWorkRect.top() + screenWorkRect.height() - appHeight;
        appLeft = screenWorkRect.left();
        appWidth = option.height;
        break;

    case static_cast<int>(SettingPosition::Right):
        if (powerStat->BatteryFull) // Not Charging, because battery is full
            appHeight = screenWorkRect.height();
        else
            appHeight = (screenWorkRect.height() * powerStat->BatteryLevel) / 100;
        if (option.align == static_cast<int>(SettingAlign::LeftTop))
            appTop = screenWorkRect.top();
        else // static_cast<int>(SettingAlign::RightBottom)
            appTop = screenWorkRect.top() + screenWorkRect.height() - appHeight;
        appLeft = screenWorkRect.left() + screenWorkRect.width() - option.height;
        appWidth = option.height;
        break;
    }

    appRect.setLeft(appLeft);
    appRect.setTop(appTop);
    appRect.setWidth(appWidth);
    appRect.setHeight(appHeight);

    setGeometry(appRect);
    updateGeometry();

#ifdef _DEBUG
    qDebug() << QString("[Monitor]");
    qDebug() << QString("Displaying on monitor %1").arg(m_option.customMonitor);
    qDebug() << QString("Base Coordinate        : (%1, %2)").arg(screenFullRect.left()).arg(screenFullRect.top());
    qDebug() << QString("Screen Resolution      : (%1, %2)").arg(screenFullRect.width()).arg(screenFullRect.height());
    qDebug() << QString("BatteryLine Coordinate : (%1, %2)").arg(appRect.left()).arg(appRect.top());
    qDebug() << QString("BatteryLine Resolution : (%1, %2)").arg(appRect.width()).arg(appRect.height());
    qDebug() << "";
#endif

#ifdef Q_OS_WIN
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    if(this->isActiveWindow() == false) {
        this->raise();
    }
#endif
}

// Must update m_batStat first
void BatteryLine::SetColor()
{
    QPalette palette;
    QColor color;
    setWindowOpacity(static_cast<qreal>(option.transparency) / 255);

    if (option.showCharge == true && powerStat->BatteryCharging == true)  // Charging, and show charge color option set
        color = option.chargeColor;
    else if (powerStat->BatteryFull == true) // Not Charging, because battery is full
        color = option.fullColor; // Even though BatteryLifePercent is not 100, consider it as 100
    else if (option.showCharge == false || powerStat->ACLineStatus == false) // Not Charging, running on battery
    {
        color = option.defaultColor;
        for (int i = 0; i < BL_COLOR_LEVEL; i++)
        {
            if (option.customEnable[i])
            {
                if (option.lowEdge[i] < powerStat->BatteryLevel && powerStat->BatteryLevel <= option.highEdge[i])
                {
                    color = option.customColor[i];
                    break;
                }
            }
        }
    }
    else
        SystemHelper::SystemError("[General] Invalid battery status data");
    color.setAlpha(255);

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
    QIcon icon = QIcon(BL_ICON);
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
    DrawLine();

    switch(reason)
    {
    case QSystemTrayIcon::Trigger: // SingleClick
        TrayMenuSetting();
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
    msgBox.setWindowIcon(QIcon(BL_ICON));
    msgBox.setWindowTitle(BL_APP_NAME);
    msgBox.setText(msgStr);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
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
    msgBox.setWindowIcon(QIcon(BL_ICON));
    msgBox.setWindowTitle(BL_APP_NAME);
    msgBox.setText(msgStr);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
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
    // SettingDialog dialog(m_option, this);
    SettingDialog dialog(option, DefaultSettings(), 0);
    connect(&dialog, SIGNAL(SignalGeneral(SettingGeneralKey, QVariant)), this, SLOT(SettingSlotGeneral(SettingGeneralKey, QVariant)));
    connect(&dialog, SIGNAL(SignalBasicColor(SettingBasicColorKey, QVariant)), this, SLOT(SettingSlotBasicColor(SettingBasicColorKey,QVariant)));
    connect(&dialog, SIGNAL(SignalCustomColor(SettingCustomColorKey, int, QVariant)), this, SLOT(SettingSlotCustomColor(SettingCustomColorKey, int, QVariant)));
    connect(&dialog, SIGNAL(SignalDefaultSetting()), this, SLOT(SettingSlotDefault()));
    dialog.exec();
    disconnect(&dialog, SIGNAL(SignalGeneral(SettingGeneralKey, QVariant)), this, SLOT(SettingSlotGeneral(SettingGeneralKey, QVariant)));
    disconnect(&dialog, SIGNAL(SignalBasicColor(SettingBasicColorKey, QVariant)), this, SLOT(SettingSlotBasicColor(SettingBasicColorKey,QVariant)));
    disconnect(&dialog, SIGNAL(SignalCustomColor(SettingCustomColorKey, int, QVariant)), this, SLOT(SettingSlotCustomColor(SettingCustomColorKey, int, QVariant)));
    disconnect(&dialog, SIGNAL(SignalDefaultSetting()), this, SLOT(SettingSlotDefault()));
}

void BatteryLine::TrayMenuPowerInfo()
{
    powerStat->Update();
    QString msgAcPower, msgCharge, msgFull;
    if (powerStat->ACLineStatus == true)
        msgAcPower = tr("AC");
    else
        msgAcPower = tr("Battery");

    if (powerStat->BatteryFull == true)
        msgCharge = tr("Full");
    else if (powerStat->BatteryCharging == true)
        msgCharge = tr("Charging");
    else if (powerStat->ACLineStatus == false)
        msgCharge = tr("Using Battery");
    else
        SystemHelper::SystemError("[General] Invalid battery status data");

    msgFull = QString("Power Source : %1\n"
                     "Battery Status : %2\n"
                     "Battery Percent : %3%\n")
            .arg(msgAcPower)
            .arg(msgCharge)
            .arg(powerStat->BatteryLevel);

    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(BL_ICON));
    msgBox.setWindowTitle(tr("Power Info"));
    msgBox.setText(msgFull);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

void BatteryLine::SettingSlotGeneral(SettingGeneralKey key, QVariant entry)
{
    switch (key)
    {
    case SettingGeneralKey::Height:
        option.height = entry.toUInt();
        break;
    case SettingGeneralKey::Position:
        option.position = entry.toUInt();
        break;
    case SettingGeneralKey::Transparency:
        option.transparency = entry.toUInt();
        break;
    case SettingGeneralKey::ShowCharge:
        option.showCharge = entry.toBool();
        break;
    case SettingGeneralKey::Align:
        option.align = entry.toUInt();
        break;
    case SettingGeneralKey::MainMonitor:
        if (entry.toBool())
            option.mainMonitor = true;
        else
            option.mainMonitor = false;
        break;
    case SettingGeneralKey::CustomMonitor:
        option.customMonitor = entry.toUInt();
        break;
    }

    DrawLine();
    WriteSettings();
}

void BatteryLine::SettingSlotBasicColor(SettingBasicColorKey key, QVariant entry)
{
    switch (key)
    {
    case SettingBasicColorKey::DefaultColor:
        option.defaultColor = entry.value<QColor>();
        break;
    case SettingBasicColorKey::ChargeColor:
        option.chargeColor = entry.value<QColor>();
        break;
    case SettingBasicColorKey::FullColor:
        option.fullColor = entry.value<QColor>();
        break;
    }

    DrawLine();
    WriteSettings();
}

void BatteryLine::SettingSlotCustomColor(SettingCustomColorKey key, int index, QVariant entry)
{
    switch (key)
    {
    case SettingCustomColorKey::Enable:
        option.customEnable[index] = entry.toBool();
        break;
    case SettingCustomColorKey::LowEdge:
        option.lowEdge[index] = entry.toInt();
        break;
    case SettingCustomColorKey::HighEdge:
        option.highEdge[index] = entry.toInt();
        break;
    case SettingCustomColorKey::Color:
        option.customColor[index] = entry.value<QColor>();
        break;
    }

    DrawLine();
    WriteSettings();
}

void BatteryLine::SettingSlotDefault()
{
    option = DefaultSettings();
    DrawLine();
    WriteSettings();
}

void BatteryLine::ReadSettings()
{
    // Default Value
    option.height = setting->value("height", 5).toUInt();
    option.position = setting->value("position", static_cast<int>(SettingPosition::Top)).toUInt();
    option.transparency = setting->value("transparency", 196).toUInt();
    option.showCharge = setting->value("showcharge", true).toBool();
    option.align = setting->value("align", static_cast<int>(SettingAlign::LeftTop)).toUInt();
    uint monitor = setting->value("monitor", static_cast<int>(SettingMonitor::Primary)).toUInt();
    if (monitor == static_cast<int>(SettingMonitor::Primary))
    {
        option.mainMonitor = true;
        option.customMonitor = 0;
    }
    else
    {
        option.mainMonitor = false;
        option.customMonitor = monitor;
    }

    setting->beginGroup("BasicColor");
    option.defaultColor = SystemHelper::RGB_QStringToQColor(setting->value("defaultcolor", SystemHelper::RGB_QColorToQString(QColor(0, 255, 0))).toString());
    option.chargeColor = SystemHelper::RGB_QStringToQColor(setting->value("chargecolor", SystemHelper::RGB_QColorToQString(QColor(0, 200, 255))).toString());
    option.fullColor = SystemHelper::RGB_QStringToQColor(setting->value("fullcolor", SystemHelper::RGB_QColorToQString(QColor(0, 162, 232))).toString());
    setting->endGroup();

    setting->beginGroup("CustomColor");
    option.customEnable[0] = setting->value("customenable1", true).toBool();
    option.customColor[0] = SystemHelper::RGB_QStringToQColor(setting->value("customcolor1", SystemHelper::RGB_QColorToQString(QColor(237, 28, 36))).toString());
    option.lowEdge[0] = setting->value("lowedge1", 0).toUInt();
    option.highEdge[0] = setting->value("highedge1", 20).toUInt();
    option.customEnable[1] = setting->value("customenable2", true).toBool();
    option.customColor[1] = SystemHelper::RGB_QStringToQColor(setting->value("customcolor2", SystemHelper::RGB_QColorToQString(QColor(255, 140, 15))).toString());
    option.lowEdge[1] = setting->value("lowedge2", 20).toUInt();
    option.highEdge[1] = setting->value("highedge2", 50).toUInt();
    for (uint i = 2; i < BL_COLOR_LEVEL; i++)
    {
        option.customEnable[i] = setting->value(QString("customenable%1").arg(i + 1), false).toBool();
        option.customColor[i] = SystemHelper::RGB_QStringToQColor(setting->value(QString("customcolor%1").arg(i + 1), SystemHelper::RGB_QColorToQString(BL_DEFAULT_DISABLED_COLOR)).toString());
        option.lowEdge[i] = setting->value(QString("lowedge%1").arg(i + 1), 0).toUInt();
        option.highEdge[i] = setting->value(QString("highedge%1").arg(i + 1), 0).toUInt();
    }
    setting->endGroup();
}

void BatteryLine::WriteSettings()
{
    setting->setValue("height", option.height);
    setting->setValue("position", option.position);
    setting->setValue("transparency", option.transparency);
    setting->setValue("showcharge", option.showCharge);
    setting->setValue("align", option.align);
    if (option.mainMonitor == true)
        setting->setValue("monitor", 0);
    else
        setting->setValue("monitor", option.customMonitor);

    setting->beginGroup("BasicColor");
    setting->setValue("defaultcolor", SystemHelper::RGB_QColorToQString(option.defaultColor));
    setting->setValue("chargecolor", SystemHelper::RGB_QColorToQString(option.chargeColor));
    setting->setValue("fullcolor", SystemHelper::RGB_QColorToQString(option.fullColor));
    setting->endGroup();

    setting->beginGroup("CustomColor");
    for (uint i = 0; i < BL_COLOR_LEVEL; i++)
    {
        // If not initial disabled value, write
        if (option.customEnable[i] || !(option.customColor[i] == BL_DEFAULT_DISABLED_COLOR && option.lowEdge[i] == 0 && option.highEdge[i] == 0))
        {
            setting->setValue(QString("customenable%1").arg(i+1), option.customEnable[i]);
            setting->setValue(QString("customcolor%1").arg(i+1), SystemHelper::RGB_QColorToQString(option.customColor[i]));
            setting->setValue(QString("lowedge%1").arg(i+1), option.lowEdge[i]);
            setting->setValue(QString("highedge%1").arg(i+1), option.highEdge[i]);
        }
    }
    setting->endGroup();
}

BL_OPTION BatteryLine::DefaultSettings()
{
    BL_OPTION option;
    memset(static_cast<void*>(&option), 0, sizeof(BL_OPTION));

    option.height = 5;
    option.position = static_cast<int>(SettingPosition::Top);
    option.transparency = 196;
    option.showCharge = true;
    option.align = static_cast<int>(SettingAlign::LeftTop);
    option.mainMonitor = true;
    option.customMonitor = 0;

    option.defaultColor = QColor(0, 255, 0);
    option.chargeColor = QColor(0, 200, 255);
    option.fullColor = QColor(0, 162, 232);

    option.customEnable[0] = true;
    option.customColor[0] = QColor(237, 28, 36);
    option.lowEdge[0] = 0;
    option.highEdge[0] = 20;
    option.customEnable[1] = true;
    option.customColor[1] = QColor(255, 140, 15);
    option.lowEdge[1] = 20;
    option.highEdge[1] = 50;
    for (uint i = 2; i < BL_COLOR_LEVEL; i++)
    {
        option.customEnable[i] = false;
        option.customColor[i] = BL_DEFAULT_DISABLED_COLOR;
        option.lowEdge[i] = 0;
        option.highEdge[i] = 0;
    }

    return option;
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
#ifdef _DEBUG
            qDebug() << "WM_POWERBROADCAST";
#endif
            DrawLine();
            break;
            /*
        case WM_DISPLAYCHANGE: // Monitor is attached or detached, Screen resolution changed, etc. Check for HMONITOR's validity.
            qDebug() << "WM_DISPLAYCHANGE";
            DrawLine();
            break;
            */
        default:
            break;
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif

