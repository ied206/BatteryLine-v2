#include "var.h"

#include "batteryline.h"
#include "ui_batteryline.h"
#include "systemhelper.h"

#include <QTimer>
#include <QDebug>
#include <QDesktopWidget>
#include <QMessageBox>

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
    m_batStat = new BatteryStatus();
#ifdef Q_OS_WIN
    m_powerNotify = new PowerNotify( hWnd);
#endif
#ifdef Q_OS_LINUX
    m_powerNotify = new PowerNotify();
#endif

    trayIconMenu = nullptr;
    trayIcon = nullptr;
    printBannerAct = nullptr;
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
    delete m_batStat;
    delete m_powerNotify;

    delete trayIconMenu;
    delete trayIcon;
    delete printBannerAct;
    delete exitAct;

    ui = nullptr;
    m_batStat = nullptr;
    m_powerNotify = nullptr;

    trayIconMenu = nullptr;
    trayIcon = nullptr;
    printBannerAct = nullptr;
    exitAct = nullptr;
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

    webBinary = QString::fromWCharArray(BL_WebBinary);
    webSource = QString::fromWCharArray(BL_WebSource);
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
    msgBox.setText(tr("BatteryLine"));
    msgBox.setInformativeText(msgStr);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    // This method crashes Linux Mint's Cinnamon Desktop
    //QMessageBox::information(this, tr("BatteryLine"), msg);
}

void BatteryLine::RedrawLine()
{
    m_batStat->GetBatteryStatus();
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
    if (m_batStat->m_BatteryFull) // Not Charging, because battery is full
        appPosSize.setWidth(mainMonitorRect.width());
    else
        appPosSize.setWidth((mainMonitorRect.width() * m_batStat->m_BatteryLevel) / 100);
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

    if (m_batStat->m_ACLineStatus == false) // Not Charging, running on battery
    {
        qDebug() << "Not Charging";
        color.setRed(0);
        color.setGreen(255);
        color.setBlue(0);
    }
    else if (m_batStat->m_BatteryFull == true) // Not Charging, because battery is full
    { // Even though BatteryLifePercent is not 100, consider it as 100
        qDebug() << "Battery Full";
        color.setRed(0);
        color.setGreen(162);
        color.setBlue(232);
    }
    else if (m_batStat->m_BatteryCharging == true)  // Charging, and show charge color option set
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
            m_batStat->GetBatteryStatus();
            SetWindowSizePos();
            SetColor();
            break;
        case WM_DISPLAYCHANGE: // Monitor is attached or detached, Screen resolution changed, etc. Check for HMONITOR's validity.
            qDebug() << "WM_DISPLAYCHANGE";
            m_batStat->GetBatteryStatus();
            SetWindowSizePos();
            SetColor();
            break;
        default:
            break;
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif
