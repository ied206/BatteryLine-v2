#include "var.h"

#include "batteryline.h"
#include "ui_batteryline.h"
#include "settingdialog.h"
#include "systemhelper.h"

#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QWindow>
#include <QScreen>
#include <QGuiApplication>

#include <cstring>


BatteryLine::BatteryLine(const bool mute, const QString helpText, QWidget *parent):
    QWidget(parent),
    ui(new Ui::BatteryLine)
{
    ui->setupUi(this);

    m_powerStat = nullptr;
    m_powerNotify = nullptr;
    m_notification = nullptr;
    m_timer = nullptr;
    m_setting = nullptr;
    m_trayIconMenu = nullptr;
    m_trayIcon = nullptr;
    m_printBannerAct = nullptr;
    m_printHelpAct = nullptr;
    m_openHomepageAct = nullptr;
    m_openSettingAct = nullptr;
    m_printPowerInfoAct = nullptr;
    m_exitAct = nullptr;
    m_muteNotifcation = mute;
    m_settingLock = false;
    memset(static_cast<void*>(&m_option), 0, sizeof(BL_OPTION));

    // Layered Window + Always On Top
    // In Windows, windows would have WS_EX_TOPMOST (0x00000008) and WS_TOPMOST | WS_CLIPCHILDREN | WS_CLIPSIBLINGS (0x86000000) styles.
    // In Cinnamon/XOrg, window which has Qt::X11BypassWindowManagerHint cannot be parent of other window.
    setEnabled(false);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

#ifdef Q_OS_WIN
    // Set Win32 Window Style and Extended Window Style directly, regardless of QWidget::setWindowFlags()
    m_hWnd = reinterpret_cast<HWND>(this->winId());
    // LONG dwExStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE); // WS_EX_TOPMOST
    // dwExStyle |= WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    SetWindowLongW(m_hWnd, GWL_STYLE, static_cast<LONG>(WS_POPUP));
    SetWindowLongW(m_hWnd, GWL_EXSTYLE, static_cast<LONG>(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED));
#endif

    // Init Member Classes
    m_powerStat = PowerStatus::CreateInstance();
    m_powerNotify = PowerNotify::CreateInstance();
    m_notification = Notification::CreateInstance();
    void* windowHandle = nullptr;
#ifdef Q_OS_WIN
    windowHandle = reinterpret_cast<void*>(m_hWnd);
#endif
    m_powerStat->Register(windowHandle);
    m_powerNotify->Register(windowHandle);
    m_notification->Register(windowHandle);

    // Connect Signal with m_powerNotify
    connect(m_powerNotify, &PowerNotify::RedrawSignal, this, &BatteryLine::DrawLine);

    // Help message will be used in PrintHelpBanner
    m_helpText = helpText;

    // Connect signal with screen change
    m_screen = QGuiApplication::primaryScreen();
    ConnectSignals(nullptr);
    if (!m_screen.isNull())
        ConnectSignals(m_screen.data());

    // Set a timer
    // Occasionally some fullscreen DirectX app hides window of BatteryLine. (e.g. Edge)
    // To mitigate this problem, call DrawLine() with QTimer regardless of power notification
    m_timer = new QTimer();
    connect(m_timer, &QTimer::timeout, this, &BatteryLine::TimerTimeout);
    m_timer->start(60 * 1000); // Per 1 minute

    // Tray Icon
    CreateTrayIcon();

    // Load Settings from INI
    m_setting = new QSettings(QSettings::IniFormat, QSettings::UserScope, BL_ORG_NAME, BL_APP_NAME);
    memset(static_cast<void*>(&m_option), 0, sizeof(BL_OPTION));
    ReadSettings();
    ChangeQScreenToSignal(TargetScreen());

    // Notification
    m_muteNotifcation = mute;

    // Set Window Size and Position, Color
    DrawLine();
}

BatteryLine::~BatteryLine()
{
    // Connect signal with screen change
    DisconnectSignals(nullptr);
    if (!m_screen.isNull())
        DisconnectSignals(m_screen.data());

    // Disconnect Signal with m_powerNotify
    disconnect(m_powerNotify, &PowerNotify::RedrawSignal, this, &BatteryLine::DrawLine);

    // Turn off QTimer
    m_timer->stop();
    disconnect(m_timer, &QTimer::timeout, this, &BatteryLine::TimerTimeout);

    // Unregister platform class instances
    m_powerStat->Unregister();
    m_powerNotify->Unregister();
    m_notification->Unregister();

    // Delete instances
    delete ui;
    delete m_powerStat;
    delete m_powerNotify;
    delete m_notification;

    delete m_timer;

    delete m_setting;

    delete m_trayIconMenu;
    delete m_trayIcon;

    delete m_printBannerAct;
    delete m_printHelpAct;
    delete m_openHomepageAct;
    delete m_openSettingAct;
    delete m_printPowerInfoAct;
    delete m_exitAct;

    // Set member variables to nullptr
    ui = nullptr;
    m_powerStat = nullptr;
    m_powerNotify = nullptr;    

    m_timer = nullptr;

    m_setting = nullptr;

    m_trayIconMenu = nullptr;
    m_trayIcon = nullptr;

    m_printBannerAct = nullptr;
    m_printHelpAct = nullptr;
    m_openHomepageAct = nullptr;
    m_openSettingAct = nullptr;
    m_printPowerInfoAct = nullptr;
    m_exitAct = nullptr;
}

void BatteryLine::DrawLine()
{
    if (m_powerStat == nullptr)
        return;

    if (!m_powerStat->Update())
        return;

    if (!m_powerStat->m_BatteryExist)
    {
        if (m_muteNotifcation)
        {
            SystemHelper::QtExit(1);
        }
        else
        {
            SystemHelper::SystemError(tr("There is no battery in this system.\nPlease attach battery and run again."));
        }
    }
    SetColor();
    SetWindowSizePos();
}

void BatteryLine::ScheduleDrawLine(int delayMs)
{
    QTimer::singleShot(delayMs, this, [this]() {
        DrawLine();
    });
}

QScreen* BatteryLine::TargetScreen() const
{
    QList<QScreen*> screens = QGuiApplication::screens();
    QScreen* primaryScreen = QGuiApplication::primaryScreen();

    if (m_option.mainMonitor)
        return primaryScreen != nullptr ? primaryScreen : (screens.isEmpty() ? nullptr : screens.first());

    if (0 <= m_option.customMonitor && m_option.customMonitor < screens.size())
        return screens.at(m_option.customMonitor);

    qWarning() << "[Monitor] Custom monitor index is unavailable:" << m_option.customMonitor;
    return primaryScreen != nullptr ? primaryScreen : (screens.isEmpty() ? nullptr : screens.first());
}

// Must update m_batStat first
void BatteryLine::SetWindowSizePos()
{
    QRect screenWorkRect; // availableGeometry - Resolution excluding taskbar
    QRect screenFullRect; // screenGeometry - Full resoultion
    QRect appRect;

    QScreen* targetScreen = TargetScreen();
    if (targetScreen == nullptr)
        return;

    // m_screen = targetScreen;
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
    qDebug().noquote() << "[Monitor]";
    qDebug().noquote() << QString("Displaying on monitor %1").arg(m_option.customMonitor);
    qDebug().noquote() << QString("Base Coordinate        : (%1, %2)").arg(screenFullRect.left()).arg(screenFullRect.top());
    qDebug().noquote() << QString("Screen Resolution      : (%1, %2)").arg(screenFullRect.width()).arg(screenFullRect.height());
    qDebug().noquote() << QString("BatteryLine Coordinate : (%1, %2)").arg(appRect.left()).arg(appRect.top());
    qDebug().noquote() << QString("BatteryLine Resolution : (%1, %2)").arg(appRect.width()).arg(appRect.height());
    qDebug().noquote() << "";
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
    {
        color = m_option.chargeColor;
    }
    else if (m_powerStat->m_BatteryFull == true) // Not Charging, because battery is full
    {
        color = m_option.fullColor; // Even though BatteryLifePercent is not 100, consider it as 100
    }
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
    {
        SystemHelper::SystemError("[General] Invalid battery status data");
    }

    color.setAlpha(255);

    palette.setColor(QPalette::Window, color);
    this->setPalette(palette);
}

// Manage QGuiApplication and QScreen Signals
void BatteryLine::ConnectSignals(QScreen* screen)
{
    if (screen == nullptr)
    { // QGuiApplication
        QGuiApplication* app = static_cast<QGuiApplication*>(QGuiApplication::instance());
        connect(app, &QGuiApplication::primaryScreenChanged, this, &BatteryLine::PrimaryScreenChanged);
        connect(app, &QGuiApplication::screenAdded, this, &BatteryLine::ScreenAdded);
        connect(app, &QGuiApplication::screenRemoved, this, &BatteryLine::ScreenRemoved);
    }
    else
    { // QScreen
        connect(screen, &QScreen::availableGeometryChanged, this, &BatteryLine::AvailableGeometryChanged);
        connect(screen, &QScreen::geometryChanged, this, &BatteryLine::GeometryChanged);
    }
}

void BatteryLine::DisconnectSignals(QScreen* screen)
{
    if (screen == nullptr)
    { // QGuiApplication
        QGuiApplication* app = static_cast<QGuiApplication*>(QGuiApplication::instance());
        disconnect(app, &QGuiApplication::primaryScreenChanged, this, &BatteryLine::PrimaryScreenChanged);
        disconnect(app, &QGuiApplication::screenAdded, this, &BatteryLine::ScreenAdded);
        disconnect(app, &QGuiApplication::screenRemoved, this, &BatteryLine::ScreenRemoved);
    }
    else
    { // QScreen
        disconnect(screen, &QScreen::availableGeometryChanged, this, &BatteryLine::AvailableGeometryChanged);
        disconnect(screen, &QScreen::geometryChanged, this, &BatteryLine::GeometryChanged);
    }
}

void BatteryLine::ChangeQScreenToSignal(QScreen* newScreen)
{
    QScreen* oldScreen = m_screen.data();
    if (oldScreen == newScreen)
        return;

    if (oldScreen != nullptr)
        DisconnectSignals(oldScreen);

    m_screen = newScreen;

    if (newScreen != nullptr)
        ConnectSignals(newScreen);
}

// QGuiApplication slots
void BatteryLine::PrimaryScreenChanged(QScreen* screen)
{
    Q_UNUSED(screen);
    ChangeQScreenToSignal(TargetScreen());
#ifdef _DEBUG
    qDebug().noquote() << "[SLOT] PrimaryScreenChanged";
    qDebug().noquote() << "";
#endif
    ScheduleDrawLine();
}

void BatteryLine::ScreenAdded(QScreen* screen)
{
    Q_UNUSED(screen);
    ChangeQScreenToSignal(TargetScreen());
#ifdef _DEBUG
    qDebug().noquote() << "[SLOT] ScreenAdded";
    qDebug().noquote() << "";
#endif
    ScheduleDrawLine();
}

void BatteryLine::ScreenRemoved(QScreen* screen)
{
    QScreen* targetScreen = TargetScreen();
    if (targetScreen == screen)
    {
        targetScreen = nullptr;
        const QList<QScreen*> screens = QGuiApplication::screens();
        for (QScreen* candidate : screens)
        {
            if (candidate != screen)
            {
                targetScreen = candidate;
                break;
            }
        }
    }
    ChangeQScreenToSignal(targetScreen);
#ifdef _DEBUG
    qDebug().noquote() << "[SLOT] ScreenRemoved";
    qDebug().noquote() << "";
#endif
    ScheduleDrawLine();
}

// QScreen slots
void BatteryLine::AvailableGeometryChanged(const QRect &geometry)
{
    Q_UNUSED(geometry);
#ifdef _DEBUG
    qDebug().noquote() << "[SLOT] AvailableGeometryChanged";
    qDebug().noquote() << "";
#endif
    DrawLine();
}

void BatteryLine::GeometryChanged(const QRect &geometry)
{
    Q_UNUSED(geometry);
#ifdef _DEBUG
    qDebug().noquote() << "[SLOT] GeometryChanged";
    qDebug().noquote() << "";
#endif
    DrawLine();
}

// QTimer slots
void BatteryLine::TimerTimeout()
{
#ifdef _DEBUG
    qDebug().noquote() << "[D] SLOT: TimerTimeout";
    qDebug().noquote() << "";
#endif
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
        QString("Joveler's BatteryLine v%1 (%2, %3)\n"
                "Show battery status as line in screen.\n\n"
                "[Homepage] %4\n\n"
                "Build %5")
            .arg(BL_VER_INST.toString(), SystemHelper::OSName(), SystemHelper::ProcArch(), BL_WEB_SOURCE, BL_REL_DATE);

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
    QDesktopServices::openUrl(QUrl(BL_WEB_SOURCE));
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
    if (!m_powerStat->Update())
        return;

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
            .arg(msgAcPower, msgCharge)
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

    if (key == SettingGeneralKey::MainMonitor || key == SettingGeneralKey::CustomMonitor)
        ChangeQScreenToSignal(TargetScreen());

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
    m_option.height = qBound(1, m_setting->value("height", 5).toInt(), 100);
    m_option.position = qBound(static_cast<int>(SettingPosition::Top), m_setting->value("position", static_cast<int>(SettingPosition::Top)).toInt(), static_cast<int>(SettingPosition::Right));
    m_option.transparency = qBound(1, m_setting->value("transparency", 196).toInt(), 255);
    m_option.showCharge = m_setting->value("showcharge", true).toBool();
    m_option.align = qBound(static_cast<int>(SettingAlign::LeftTop), m_setting->value("align", static_cast<int>(SettingAlign::LeftTop)).toInt(), static_cast<int>(SettingAlign::RightBottom));
    int monitor = m_setting->value("monitor", static_cast<int>(SettingMonitor::Primary)).toInt();
    if (monitor <= static_cast<int>(SettingMonitor::Primary))
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
    m_option.lowEdge[0] = qBound(0, m_setting->value("lowedge1", 0).toInt(), 100);
    m_option.highEdge[0] = qBound(0, m_setting->value("highedge1", 20).toInt(), 100);
    m_option.customEnable[1] = m_setting->value("customenable2", true).toBool();
    m_option.customColor[1] = SystemHelper::RGB_QStringToQColor(m_setting->value("customcolor2", SystemHelper::RGB_QColorToQString(QColor(255, 140, 15))).toString());
    m_option.lowEdge[1] = qBound(0, m_setting->value("lowedge2", 20).toInt(), 100);
    m_option.highEdge[1] = qBound(0, m_setting->value("highedge2", 50).toInt(), 100);
    for (uint i = 2; i < BL_COLOR_LEVEL; i++)
    {
        m_option.customEnable[i] = m_setting->value(QString("customenable%1").arg(i + 1), false).toBool();
        m_option.customColor[i] = SystemHelper::RGB_QStringToQColor(m_setting->value(QString("customcolor%1").arg(i + 1), SystemHelper::RGB_QColorToQString(BL_DEFAULT_DISABLED_COLOR)).toString());
        m_option.lowEdge[i] = qBound(0, m_setting->value(QString("lowedge%1").arg(i + 1), 0).toInt(), 100);
        m_option.highEdge[i] = qBound(0, m_setting->value(QString("highedge%1").arg(i + 1), 0).toInt(), 100);
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
bool BatteryLine::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    if (eventType == "windows_generic_MSG")
    {
        MSG *msg = static_cast<MSG*>(message);

        switch (msg->message)
        {
        case WM_POWERBROADCAST:
#ifdef _DEBUG
            qDebug().noquote() << "[WM] WM_POWERBROADCAST";
            qDebug().noquote() << "";
#endif
            switch (msg->wParam)
            {
            case PBT_APMSUSPEND:
                if (m_timer != nullptr)
                    m_timer->stop();
                break;
            case PBT_APMRESUMEAUTOMATIC:
            case PBT_APMRESUMESUSPEND:
                if (m_timer != nullptr && !m_timer->isActive())
                    m_timer->start(60 * 1000);
                ScheduleDrawLine(1000);
                ScheduleDrawLine(5000);
                break;
            case PBT_APMPOWERSTATUSCHANGE:
            case PBT_POWERSETTINGCHANGE:
                ScheduleDrawLine();
                break;
            default:
                break;
            }
            break;
        case WM_DISPLAYCHANGE: // Monitor is attached or detached, Screen resolution changed, etc. Check for HMONITOR's validity.
#ifdef _DEBUG
            qDebug().noquote() << "[WM] WM_DISPLAYCHANGE";
            qDebug().noquote() << "";
#endif
            ChangeQScreenToSignal(TargetScreen());
            ScheduleDrawLine();
            ScheduleDrawLine(1000);
            break;
        default:
            break;
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif


