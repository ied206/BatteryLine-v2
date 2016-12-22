#ifndef BATTERYLINE_H
#define BATTERYLINE_H

// sudo apt install libgl-dev

#include "Var.h"

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
    explicit BatteryLine(QWidget *parent = 0);
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

    void DrawLine();

    void SettingGeneral(SettingGeneralKey key, QVariant entry);
    void SettingBasicColor(SettingBasicColorKey key, QVariant entry);
    void SettingCustomColor(SettingCustomColorKey key, int index, QVariant entry);


private:
    Ui::BatteryLine *ui;
    void RegisterPowerNotification();
    void UnregisterPowerNotification();
    void SetWindowSizePos();
    void SetColor();
    void CreateTrayIcon();
    QString GetIniFullPath();
    void ReadSettings();
    void WriteSettings(bool defaultValue = false);

    PowerNotify* m_powerNotify;
    PowerStatus* m_powerStat;

    QSettings* m_setting;
    BL_OPTION m_option;

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
