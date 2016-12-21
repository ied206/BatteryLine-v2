#ifndef BATTERYLINE_H
#define BATTERYLINE_H

// sudo apt install libgl-dev

#include <QWidget>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QString>

#ifdef Q_OS_WIN
#include "platform/win/powernotify-win.h"
#include "platform/win/powerstatus-win.h"
#endif
#ifdef Q_OS_LINUX
#include "platform/linux/powernotify-linux.h"
#include "platform/linux/powerstatus-linux.h"
#endif

namespace Ui {
    class BatteryLine;
}

class BatteryLine : public QWidget
{
    Q_OBJECT

public:
    explicit BatteryLine(QWidget *parent = 0);
    ~BatteryLine();

protected:
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
#endif

private slots:
    void TrayIconClicked(QSystemTrayIcon::ActivationReason reason);
    void TrayMenuPrintBanner();
    void TrayMenuPrintHelp();
    void TrayMenuHomepage();
    void TrayMenuLicense();
    void TrayMenuSetting();
    void TrayMenuPowerInfo();
    void TrayMenuExit();

    void RedrawLine();

private:
    Ui::BatteryLine *ui;
    void RegisterPowerNotification();
    void UnregisterPowerNotification();
    void SetWindowSizePos();
    void SetColor();
    void CreateTrayIcon();
    QString GetIniFullPath();

    PowerNotify* m_powerNotify;
    PowerStatus* m_powerStat;

    QMenu* trayIconMenu;
    QSystemTrayIcon* trayIcon;

    QAction* printBannerAct;
    QAction* printHelpAct;
    QAction* openHomepageAct;
    QAction* openLicenseAct;
    QAction* openSettingAct;
    QAction* printPowerInfoAct;
    QAction* exitAct;

#ifdef Q_OS_WIN
    HWND hWnd;
#endif
};

#endif // BATTERYLINE_H
