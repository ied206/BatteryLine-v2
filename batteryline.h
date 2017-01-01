#ifndef BATTERYLINE_H
#define BATTERYLINE_H

// sudo apt install libgl-dev

#include "var.h"

#include <QWidget>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QString>
#include <QSettings>

#include "settingdialog.h"

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
    explicit BatteryLine(bool muteNotifcation, QWidget *parent = 0);
    ~BatteryLine();

protected:
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
#endif

private slots:
    void PrimaryScreenChanged();
    void ScreenCountChanged(int newCount);
    void ScreenResized(int screen);
    void ScreenWorkAreaResized(int screen);

    void TrayIconClicked(QSystemTrayIcon::ActivationReason reason);
    void TrayMenuPrintBanner();
    void TrayMenuPrintHelp();
    void TrayMenuHomepage();
    void TrayMenuLicense();
    void TrayMenuSetting();
    void TrayMenuPowerInfo();
    void TrayMenuExit();

    void SettingSlotGeneral(SettingGeneralKey key, QVariant entry);
    void SettingSlotBasicColor(SettingBasicColorKey key, QVariant entry);
    void SettingSlotCustomColor(SettingCustomColorKey key, int index, QVariant entry);
    void SettingSlotDefault();

private:
    Ui::BatteryLine *ui;
    void DrawLine();
    void SetWindowSizePos();
    void SetColor();
    void CreateTrayIcon();
    QString GetIniFullPath();
    void ReadSettings();
    void WriteSettings();
    BL_OPTION DefaultSettings();

    PowerNotify* powerNotify;
    PowerStatus* powerStat;

    QSettings* setting;
    BL_OPTION option;

    QMenu* trayIconMenu;
    QSystemTrayIcon* trayIcon;

    QAction* printBannerAct;
    QAction* printHelpAct;
    QAction* openHomepageAct;
    QAction* openLicenseAct;
    QAction* openSettingAct;
    QAction* printPowerInfoAct;
    QAction* exitAct;

    bool muteNotifcation;

#ifdef Q_OS_WIN
    HWND hWnd;
#endif
};

#endif // BATTERYLINE_H
