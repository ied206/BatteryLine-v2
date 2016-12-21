#include "var.h"

#include "batteryline.h"
#include "ui_batteryline.h"
#include "systemhelper.h"

#include <QTimer>
#include <QDebug>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

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
    dwExStyle |= WS_EX_NOACTIVATE;
    SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);
#endif

    // Init Member Variables
    m_powerStat = new PowerStatus();
#ifdef Q_OS_WIN
    m_powerNotify = new PowerNotify(hWnd);
#endif
#ifdef Q_OS_LINUX
    m_powerNotify = new PowerNotify();
#endif

    trayIconMenu = nullptr;
    trayIcon = nullptr;

    printBannerAct = nullptr;
    printHelpAct = nullptr;
    openHomepageAct = nullptr;
    openLicenseAct = nullptr;
    openSettingAct = nullptr;
    printPowerInfoAct = nullptr;
    exitAct = nullptr;

    // Connect Signal with m_powerNotify
    connect(m_powerNotify, &PowerNotify::RedrawSignal, this, &BatteryLine::RedrawLine);

    // Tray Icon
    CreateTrayIcon();

    // Set Windows Size and Position, Color
    setAutoFillBackground(true);
    RedrawLine();
}

BatteryLine::~BatteryLine()
{
    delete ui;
    delete m_powerStat;
    delete m_powerNotify;

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

void BatteryLine::RedrawLine()
{
    m_powerStat->Update();
    SetColor();
    SetWindowSizePos();
}

// Must update m_batStat first
void BatteryLine::SetWindowSizePos()
{
    QRect mainMonitorRect = QApplication::desktop()->availableGeometry(-1); // -1 == Main Monitor
    QRect appPosSize;
    appPosSize.setLeft(mainMonitorRect.left());
    appPosSize.setTop(mainMonitorRect.top());
    if (m_powerStat->m_BatteryFull) // Not Charging, because battery is full
        appPosSize.setWidth(mainMonitorRect.width());
    else
        appPosSize.setWidth((mainMonitorRect.width() * m_powerStat->m_BatteryLevel) / 100);
    appPosSize.setHeight(5);

    qDebug() << "[Monitor]";
    qDebug() << "Left   : " << appPosSize.left();
    qDebug() << "Top    : " << appPosSize.top();
    qDebug() << "Width  : " << appPosSize.width();
    qDebug() << "Height : " << appPosSize.height() << "\n";

#ifdef Q_OS_WIN
    // Evade TaskBar if option is set
    RECT trayPos;
    HWND hTray = FindWindowW(L"Shell_TrayWnd", NULL);
    if (hTray == NULL || GetWindowRect(hTray, &trayPos) == 0)
        SystemHelper::SystemError(tr("Cannot get taskbar's position"));
#endif

    setGeometry(appPosSize);
    updateGeometry();
}

// Must update m_batStat first
void BatteryLine::SetColor()
{
    QPalette palette;
    QColor color;
    color.setAlpha(255);

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
    case QSystemTrayIcon::Context: // RightClick
        qDebug() << "Context";
        break;
    case QSystemTrayIcon::DoubleClick:
        qDebug() << "DoubleClick";
        break;
    case QSystemTrayIcon::Trigger: // SingleClick
        qDebug() << "Trigger";
        TrayMenuPrintBanner();
        break;
    case QSystemTrayIcon::MiddleClick:
        qDebug() << "MiddleClick";
        break;
    case QSystemTrayIcon::Unknown:
    default:
        qDebug() << "Unknown";
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

    webBinary = QString::fromWCharArray(BL_WEB_BINARY);
    webSource = QString::fromWCharArray(BL_WEB_SOURCE);
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
    msgBox.setText(QString::fromWCharArray(BL_PROG_NAME));
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

    webBinary = QString::fromWCharArray(BL_WEB_BINARY);
    webSource = QString::fromWCharArray(BL_WEB_SOURCE);
    msgStr = QString("[BatteryLine Help Message]\n"
                  "Show battery status as line in screen.\n\n"
                  "[Command Line Option]\n"
                 "-q : Launch this program without notification.\n"
                 "-h : Print this help message and exit.\n\n"
                 "[Setting]\n"
                 "You can edit BatteryLine's setting in BatteryLine.ini.");

    QMessageBox msgBox;
    msgBox.setText(QString::fromWCharArray(BL_PROG_NAME));
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
    QDesktopServices::openUrl(QUrl(QString::fromWCharArray(BL_WEB_BINARY)));
}

void BatteryLine::TrayMenuLicense()
{
    // Open GitHub repository's LICENSE page
    QDesktopServices::openUrl(QUrl(QString::fromWCharArray(BL_WEB_LICENSE)));
}

void BatteryLine::TrayMenuSetting()
{
    // Open BatteryLine.ini
    QDesktopServices::openUrl(QUrl(GetIniFullPath()));
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

QString BatteryLine::GetIniFullPath()
{
    QString path = QCoreApplication::applicationDirPath();
    path.append(tr("/")); // Qt will translate / to \ if it runs on Windows
    path.append(QString::fromWCharArray(BL_SETTING_FILE));
    return path;
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
        case WM_POWERBROADCAST:
            qDebug() << "WM_POWERBROADCAST";
            RedrawLine();
            break;
        case WM_DISPLAYCHANGE: // Monitor is attached or detached, Screen resolution changed, etc. Check for HMONITOR's validity.
            qDebug() << "WM_DISPLAYCHANGE";
            RedrawLine();
            break;
        default:
            break;
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif
