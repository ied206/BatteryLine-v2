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
#include <QWindow>
#include <QScreen>
#include <QGuiApplication>


BatteryLine::BatteryLine(const bool mute, const QString helpText, QWidget *parent):
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
    m_hWnd = reinterpret_cast<HWND>(this->winId());
    LONG dwExStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);
    dwExStyle |= WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    SetWindowLongW(m_hWnd, GWL_EXSTYLE, dwExStyle);
#endif

    // Init Member Classes
    m_powerStat = new PowerStatus();
#ifdef Q_OS_WIN
    m_powerNotify = new PowerNotify(m_hWnd);
    m_notification = new Notification(m_hWnd);
#endif
#ifdef Q_OS_LINUX
    m_powerNotify = new PowerNotify();
    m_notification = new Notification();
#endif
    // Connect Signal with m_powerNotify
    connect(m_powerNotify, &PowerNotify::RedrawSignal, this, &BatteryLine::DrawLine);

    // Init Member Variables - Setting and Context Menu
    m_setting = nullptr;
    m_settingLock = false;

    m_trayIconMenu = nullptr;
    m_trayIcon = nullptr;

    m_printBannerAct = nullptr;
    m_printHelpAct = nullptr;
    m_openHomepageAct = nullptr;
    m_openLicenseAct = nullptr;
    m_openSettingAct = nullptr;
    m_printPowerInfoAct = nullptr;
    m_exitAct = nullptr;

    // Help message will be used in PrintHelpBanner
    m_helpText = helpText;

    // Connect signal with screen change
    QScreen* screen = QGuiApplication::primaryScreen();
    connect(screen, &QScreen::availableGeometryChanged, this, &BatteryLine::AvailableGeometryChanged);
    // connect(QApplication::desktop(), &QDesktopWidget::primaryScreenChanged, this, &BatteryLine::PrimaryScreenChanged);
    // connect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, this, &BatteryLine::ScreenCountChanged);
    // connect(QApplication::desktop(), &QDesktopWidget::resized, this, &BatteryLine::ScreenResized);
    // connect(QApplication::desktop(), &QDesktopWidget::workAreaResized, this, &BatteryLine::ScreenWorkAreaResized);

    // Tray Icon
    CreateTrayIcon();

    // Load Settings from INI
    m_setting = new QSettings(QSettings::IniFormat, QSettings::UserScope, BL_ORG_NAME, BL_APP_NAME);
    memset(static_cast<void*>(&m_option), 0, sizeof(BL_OPTION));
    ReadSettings();

    // Notification
    m_muteNotifcation = mute;

    // Set Window Size and Position, Color
    DrawLine();
}

BatteryLine::~BatteryLine()
{
    // Connect signal with screen change
    QScreen* screen = QGuiApplication::primaryScreen();
    disconnect(screen, &QScreen::availableGeometryChanged, this, &BatteryLine::AvailableGeometryChanged);
    /*
    disconnect(QApplication::desktop(), &QDesktopWidget::primaryScreenChanged, this, &BatteryLine::PrimaryScreenChanged);
    disconnect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, this, &BatteryLine::ScreenCountChanged);
    disconnect(QApplication::desktop(), &QDesktopWidget::resized, this, &BatteryLine::ScreenResized);
    disconnect(QApplication::desktop(), &QDesktopWidget::workAreaResized, this, &BatteryLine::ScreenWorkAreaResized);
    */

    // Disconnect Signal with m_powerNotify
    disconnect(m_powerNotify, &PowerNotify::RedrawSignal, this, &BatteryLine::DrawLine);

    delete ui;
    delete m_powerStat;
    delete m_powerNotify;
    delete m_notification;

    delete m_setting;

    delete m_trayIconMenu;
    delete m_trayIcon;

    delete m_printBannerAct;
    delete m_printHelpAct;
    delete m_openHomepageAct;
    delete m_openLicenseAct;
    delete m_openSettingAct;
    delete m_printPowerInfoAct;
    delete m_exitAct;

    ui = nullptr;
    m_powerStat = nullptr;
    m_powerNotify = nullptr;

    m_setting = nullptr;

    m_trayIconMenu = nullptr;
    m_trayIcon = nullptr;

    m_printBannerAct = nullptr;
    m_printHelpAct = nullptr;
    m_openHomepageAct = nullptr;
    m_openLicenseAct = nullptr;
    m_openSettingAct = nullptr;
    m_printPowerInfoAct = nullptr;
    m_exitAct = nullptr;
}

void BatteryLine::DrawLine()
{
    m_powerStat->Update();
    if (!m_powerStat->m_BatteryExist)
    {
        if (m_muteNotifcation)
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
    QScreen* targetScreen;
    QRect screenWorkRect; // availableGeometry - Resolution excluding taskbar
    QRect screenFullRect; // screenGeometry - Full resoultion
    QRect appRect;

    if (m_option.mainMonitor == true) // Main Monitor
    {
        targetScreen = QGuiApplication::primaryScreen();
    }
    else // Custom Monitor
    {
        QList<QScreen*> screens = QGuiApplication::screens();
        if (m_option.customMonitor < 0 || screens.size() <= m_option.customMonitor)
            targetScreen = screens.at(m_option.customMonitor);
        else // Silently fallback to primary monitor
            targetScreen = QGuiApplication::primaryScreen();
    }

    screenWorkRect = targetScreen->availableGeometry();
    screenFullRect = targetScreen->geometry();

    int appLeft = 0;
    int appTop = 0;
    int appWidth = 0;
    int appHeight = 0;

    switch (m_option.position)
    {
    case static_cast<int>(SettingPosition::Top):
        if (m_powerStat->m_BatteryFull) // Not Charging, because battery is full
            appWidth = screenWorkRect.width();
        else
            appWidth = (screenWorkRect.width() * m_powerStat->m_BatteryLevel) / 100;
        if (m_option.align == static_cast<int>(SettingAlign::LeftTop))
            appLeft = screenWorkRect.left();
        else // static_cast<int>(SettingAlign::RightBottom)
            appLeft = screenWorkRect.left() + screenWorkRect.width() - appWidth;
        appTop = screenWorkRect.top();
        appHeight = m_option.height;
        break;

    case static_cast<int>(SettingPosition::Bottom):
        if (m_powerStat->m_BatteryFull) // Not Charging, because battery is full
            appWidth = screenWorkRect.width();
        else
            appWidth = (screenWorkRect.width() * m_powerStat->m_BatteryLevel) / 100;
        if (m_option.align == static_cast<int>(SettingAlign::LeftTop))
            appLeft = screenWorkRect.left();
        else // static_cast<int>(SettingAlign::RightBottom)
            appLeft = screenWorkRect.left() + screenWorkRect.width() - appWidth;
        appTop = screenWorkRect.top() + screenWorkRect.height() - m_option.height;
        appHeight = m_option.height;
        break;

    case static_cast<int>(SettingPosition::Left):
        if (m_powerStat->m_BatteryFull) // Not Charging, because battery is full
            appHeight = screenWorkRect.height();
        else
            appHeight = (screenWorkRect.height() * m_powerStat->m_BatteryLevel) / 100;
        if (m_option.align == static_cast<int>(SettingAlign::LeftTop))
            appTop = screenWorkRect.top();
        else // static_cast<int>(SettingAlign::RightBottom)
            appTop = screenWorkRect.top() + screenWorkRect.height() - appHeight;
        appLeft = screenWorkRect.left();
        appWidth = m_option.height;
        break;

    case static_cast<int>(SettingPosition::Right):
        if (m_powerStat->m_BatteryFull) // Not Charging, because battery is full
            appHeight = screenWorkRect.height();
        else
            appHeight = (screenWorkRect.height() * m_powerStat->m_BatteryLevel) / 100;
        if (m_option.align == static_cast<int>(SettingAlign::LeftTop))
            appTop = screenWorkRect.top();
        else // static_cast<int>(SettingAlign::RightBottom)
            appTop = screenWorkRect.top() + screenWorkRect.height() - appHeight;
        appLeft = screenWorkRect.left() + screenWorkRect.width() - m_option.height;
        appWidth = m_option.height;
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
    SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#endif
}

// Must update m_batStat first
void BatteryLine::SetColor()
{
    QPalette palette;
    QColor color;
    setWindowOpacity(static_cast<qreal>(m_option.transparency) / 255);

    if (m_option.showCharge == true && m_powerStat->m_BatteryCharging == true)  // Charging, and show charge color option set
        color = m_option.chargeColor;
    else if (m_powerStat->m_BatteryFull == true) // Not Charging, because battery is full
        color = m_option.fullColor; // Even though BatteryLifePercent is not 100, consider it as 100
    else if (m_option.showCharge == false || m_powerStat->m_ACLineStatus == false) // Not Charging, running on battery
    {
        color = m_option.defaultColor;
        for (int i = 0; i < BL_COLOR_LEVEL; i++)
        {
            if (m_option.customEnable[i])
            {
                if (m_option.lowEdge[i] < m_powerStat->m_BatteryLevel && m_powerStat->m_BatteryLevel <= m_option.highEdge[i])
                {
                    color = m_option.customColor[i];
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

// Obsolete QDesktopWidget slots
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

// QScreen slots
void BatteryLine::AvailableGeometryChanged(const QRect &geometry)
{
    (void) geometry; // Remove unsued parameter warning
    DrawLine();
}

// http://doc.qt.io/qt-5/qtwidgets-mainwindows-menus-example.html
void BatteryLine::CreateTrayIcon()
{
    // Init
    m_trayIconMenu = new QMenu(this);
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayIconMenu);

    // Icon
    QIcon icon = QIcon(BL_ICON);
    m_trayIcon->setIcon(icon);
    setWindowIcon(icon);

    // Context Menu
    m_printBannerAct = new QAction(tr("A&bout"), this);
    m_printBannerAct->setIcon(icon);
    connect(m_printBannerAct, &QAction::triggered, this, &BatteryLine::TrayMenuPrintBanner);
    m_trayIconMenu->addAction(m_printBannerAct);

    m_printHelpAct = new QAction(tr("&Help"), this);
    connect(m_printHelpAct, &QAction::triggered, this, &BatteryLine::TrayMenuPrintHelp);
    m_trayIconMenu->addAction(m_printHelpAct);
    m_trayIconMenu->addSeparator();

    m_openHomepageAct = new QAction(tr("&Homepage"), this);
    connect(m_openHomepageAct, &QAction::triggered, this, &BatteryLine::TrayMenuHomepage);
    m_trayIconMenu->addAction(m_openHomepageAct);

    m_openLicenseAct = new QAction(tr("&License"), this);
    connect(m_openLicenseAct, &QAction::triggered, this, &BatteryLine::TrayMenuLicense);
    m_trayIconMenu->addAction(m_openLicenseAct);
    m_trayIconMenu->addSeparator();

    m_openSettingAct = new QAction(tr("&Setting"), this);
    connect(m_openSettingAct, &QAction::triggered, this, &BatteryLine::TrayMenuSetting);
    m_trayIconMenu->addAction(m_openSettingAct);

    m_printPowerInfoAct = new QAction(tr("&Power Info"), this);
    connect(m_printPowerInfoAct, &QAction::triggered, this, &BatteryLine::TrayMenuPowerInfo);
    m_trayIconMenu->addAction(m_printPowerInfoAct);
    m_trayIconMenu->addSeparator();

    m_exitAct = new QAction(tr("E&xit"), this);
    connect(m_exitAct, &QAction::triggered, this, &BatteryLine::TrayMenuExit);
    m_trayIconMenu->addAction(m_exitAct);

    // Event Handler
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &BatteryLine::TrayIconClicked);

    // Show
    m_trayIcon->show();
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
        break;
    }
}

void BatteryLine::TrayMenuExit()
{
    QCoreApplication::exit(0);
}

void BatteryLine::TrayMenuPrintBanner()
{
    QString msgStr =
        QString("Joveler's BatteryLine v%1.%2 (%3, %4bit)\n"
                "Show battery status as line in screen.\n\n"
                "[Binary] %5\n"
                "[Source] %6\n\n"
                "Build %7")
        .arg(BL_MAJOR_VER)
        .arg(BL_MINOR_VER)
        .arg(SystemHelper::ArchOS())
        .arg(SystemHelper::ArchBit())
        .arg(BL_WEB_BINARY)
        .arg(BL_WEB_SOURCE)
        .arg(BL_REL_DATE);

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
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(BL_ICON));
    msgBox.setWindowTitle(BL_APP_NAME);
    msgBox.setText(QString(m_helpText));
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
    if (!m_settingLock)
    {
        m_settingLock = true;

        SettingDialog dialog(m_option, DefaultSettings(), nullptr);
        connect(&dialog, &SettingDialog::SignalGeneral, this, &BatteryLine::SettingSlotGeneral);
        connect(&dialog, &SettingDialog::SignalBasicColor, this, &BatteryLine::SettingSlotBasicColor);
        connect(&dialog, &SettingDialog::SignalCustomColor, this, &BatteryLine::SettingSlotCustomColor);
        connect(&dialog, &SettingDialog::SignalDefaultSetting, this, &BatteryLine::SettingSlotDefault);
        dialog.exec();
        disconnect(&dialog, &SettingDialog::SignalGeneral, this, &BatteryLine::SettingSlotGeneral);
        disconnect(&dialog, &SettingDialog::SignalBasicColor, this, &BatteryLine::SettingSlotBasicColor);
        disconnect(&dialog, &SettingDialog::SignalCustomColor, this, &BatteryLine::SettingSlotCustomColor);
        disconnect(&dialog, &SettingDialog::SignalDefaultSetting, this, &BatteryLine::SettingSlotDefault);

        m_settingLock = false;
    }
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
        m_option.height = entry.toInt();
        break;
    case SettingGeneralKey::Position:
        m_option.position = entry.toInt();
        break;
    case SettingGeneralKey::Transparency:
        m_option.transparency = entry.toInt();
        break;
    case SettingGeneralKey::ShowCharge:
        m_option.showCharge = entry.toBool();
        break;
    case SettingGeneralKey::Align:
        m_option.align = entry.toInt();
        break;
    case SettingGeneralKey::MainMonitor:
        m_option.mainMonitor = entry.toBool();
        break;
    case SettingGeneralKey::CustomMonitor:
        m_option.customMonitor = entry.toInt();
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
        m_option.defaultColor = entry.value<QColor>();
        break;
    case SettingBasicColorKey::ChargeColor:
        m_option.chargeColor = entry.value<QColor>();
        break;
    case SettingBasicColorKey::FullColor:
        m_option.fullColor = entry.value<QColor>();
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
        m_option.customEnable[index] = entry.toBool();
        break;
    case SettingCustomColorKey::LowEdge:
        m_option.lowEdge[index] = entry.toInt();
        break;
    case SettingCustomColorKey::HighEdge:
        m_option.highEdge[index] = entry.toInt();
        break;
    case SettingCustomColorKey::Color:
        m_option.customColor[index] = entry.value<QColor>();
        break;
    }

    DrawLine();
    WriteSettings();
}

void BatteryLine::SettingSlotDefault()
{
    m_option = DefaultSettings();
    DrawLine();
    WriteSettings();
}

void BatteryLine::ReadSettings()
{
    // Default Value
    m_option.height = m_setting->value("height", 5).toInt();
    m_option.position = m_setting->value("position", static_cast<int>(SettingPosition::Top)).toInt();
    m_option.transparency = m_setting->value("transparency", 196).toInt();
    m_option.showCharge = m_setting->value("showcharge", true).toBool();
    m_option.align = m_setting->value("align", static_cast<int>(SettingAlign::LeftTop)).toInt();
    int monitor = m_setting->value("monitor", static_cast<int>(SettingMonitor::Primary)).toInt();
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

    m_setting->beginGroup("BasicColor");
    m_option.defaultColor = SystemHelper::RGB_QStringToQColor(m_setting->value("defaultcolor", SystemHelper::RGB_QColorToQString(QColor(0, 255, 0))).toString());
    m_option.chargeColor = SystemHelper::RGB_QStringToQColor(m_setting->value("chargecolor", SystemHelper::RGB_QColorToQString(QColor(0, 200, 255))).toString());
    m_option.fullColor = SystemHelper::RGB_QStringToQColor(m_setting->value("fullcolor", SystemHelper::RGB_QColorToQString(QColor(0, 162, 232))).toString());
    m_setting->endGroup();

    m_setting->beginGroup("CustomColor");
    m_option.customEnable[0] = m_setting->value("customenable1", true).toBool();
    m_option.customColor[0] = SystemHelper::RGB_QStringToQColor(m_setting->value("customcolor1", SystemHelper::RGB_QColorToQString(QColor(237, 28, 36))).toString());
    m_option.lowEdge[0] = m_setting->value("lowedge1", 0).toInt();
    m_option.highEdge[0] = m_setting->value("highedge1", 20).toInt();
    m_option.customEnable[1] = m_setting->value("customenable2", true).toBool();
    m_option.customColor[1] = SystemHelper::RGB_QStringToQColor(m_setting->value("customcolor2", SystemHelper::RGB_QColorToQString(QColor(255, 140, 15))).toString());
    m_option.lowEdge[1] = m_setting->value("lowedge2", 20).toInt();
    m_option.highEdge[1] = m_setting->value("highedge2", 50).toInt();
    for (uint i = 2; i < BL_COLOR_LEVEL; i++)
    {
        m_option.customEnable[i] = m_setting->value(QString("customenable%1").arg(i + 1), false).toBool();
        m_option.customColor[i] = SystemHelper::RGB_QStringToQColor(m_setting->value(QString("customcolor%1").arg(i + 1), SystemHelper::RGB_QColorToQString(BL_DEFAULT_DISABLED_COLOR)).toString());
        m_option.lowEdge[i] = m_setting->value(QString("lowedge%1").arg(i + 1), 0).toInt();
        m_option.highEdge[i] = m_setting->value(QString("highedge%1").arg(i + 1), 0).toInt();
    }
    m_setting->endGroup();
}

void BatteryLine::WriteSettings()
{
    m_setting->setValue("height", m_option.height);
    m_setting->setValue("position", m_option.position);
    m_setting->setValue("transparency", m_option.transparency);
    m_setting->setValue("showcharge", m_option.showCharge);
    m_setting->setValue("align", m_option.align);
    if (m_option.mainMonitor)
        m_setting->setValue("monitor", 0);
    else
        m_setting->setValue("monitor", m_option.customMonitor);

    m_setting->beginGroup("BasicColor");
    m_setting->setValue("defaultcolor", SystemHelper::RGB_QColorToQString(m_option.defaultColor));
    m_setting->setValue("chargecolor", SystemHelper::RGB_QColorToQString(m_option.chargeColor));
    m_setting->setValue("fullcolor", SystemHelper::RGB_QColorToQString(m_option.fullColor));
    m_setting->endGroup();

    m_setting->beginGroup("CustomColor");
    for (uint i = 0; i < BL_COLOR_LEVEL; i++)
    {
        // If not initial disabled value, write
        if (m_option.customEnable[i] || !(m_option.customColor[i] == BL_DEFAULT_DISABLED_COLOR && m_option.lowEdge[i] == 0 && m_option.highEdge[i] == 0))
        {
            m_setting->setValue(QString("customenable%1").arg(i+1), m_option.customEnable[i]);
            m_setting->setValue(QString("customcolor%1").arg(i+1), SystemHelper::RGB_QColorToQString(m_option.customColor[i]));
            m_setting->setValue(QString("lowedge%1").arg(i+1), m_option.lowEdge[i]);
            m_setting->setValue(QString("highedge%1").arg(i+1), m_option.highEdge[i]);
        }
    }
    m_setting->endGroup();
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
        case WM_DISPLAYCHANGE: // Monitor is attached or detached, Screen resolution changed, etc. Check for HMONITOR's validity.
#ifdef _DEBUG
            qDebug() << "WM_DISPLAYCHANGE";
#endif
            DrawLine();
            break;
        default:
            break;
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif


