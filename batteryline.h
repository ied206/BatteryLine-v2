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
#include "platform/win/notification-win.h"
#include "platform/win/powernotify-win.h"
#include "platform/win/powerstatus-win.h"
#endif
#ifdef Q_OS_LINUX
#include "platform/linux/notification-linux.h"
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
    explicit BatteryLine(const bool mute, const QString helpText, QWidget *parent = nullptr);
    ~BatteryLine();

protected:
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
#endif

private slots:
    // Obsolete QDesktopWidget slots
    void PrimaryScreenChanged();
    void ScreenCountChanged(int newCount);
    void ScreenResized(int screen);
    void ScreenWorkAreaResized(int screen);
    // QScreen slots
    void AvailableGeometryChanged(const QRect &geometry);

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

    Notification* m_notification;
    PowerNotify* m_powerNotify;
    PowerStatus* m_powerStat;

    QSettings* m_setting;
    BL_OPTION m_option;

    QMenu* m_trayIconMenu;
    QSystemTrayIcon* m_trayIcon;

    QAction* m_printBannerAct;
    QAction* m_printHelpAct;
    QAction* m_openHomepageAct;
    QAction* m_openLicenseAct;
    QAction* m_openSettingAct;
    QAction* m_printPowerInfoAct;
    QAction* m_exitAct;

    bool m_muteNotifcation;
    bool m_settingLock;

    QString m_helpText;

#ifdef Q_OS_WIN
    HWND m_hWnd;
#endif
};

#endif // BATTERYLINE_H
