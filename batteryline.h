#ifndef BATTERYLINE_H
#define BATTERYLINE_H

// sudo apt install libgl-dev

#include <QWidget>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QString>
#include <QSettings>

#include "settingdialog.h"

#include "platform/notification.h"
#include "platform/powernotify.h"
#include "platform/powerstatus.h"

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
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result);
#endif

private slots:
    // QGuiApplication slots
    void PrimaryScreenChanged(QScreen* screen);
    void ScreenAdded(QScreen* screen);
    void ScreenRemoved(QScreen* screen);
    // QScreen slots
    void AvailableGeometryChanged(const QRect &geometry);
    void GeometryChanged(const QRect &geometry);
    // QTimer slots
    void TimerTimeout();

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
    // DrawLine Series
    void DrawLine();
    void SetWindowSizePos();
    void SetColor();
    // Manage QGuiApplication and QScreen Signals
    void ConnectSignals(QScreen* screen = nullptr);
    void DisconnectSignals(QScreen* screen = nullptr);
    void ChangeQScreenToSignal(QScreen* newScreen);
    // TrayMenu
    void CreateTrayIcon();
    // Manage Settings
    QString GetIniFullPath();
    void ReadSettings();
    void WriteSettings();
    BL_OPTION DefaultSettings();

// Member Variables
    // QScreen slot sender
    QScreen* m_screen;

    // Power Notification
    Notification* m_notification;
    PowerNotify* m_powerNotify;
    PowerStatus* m_powerStat;

    // Timer
    QTimer* m_timer;

    // Setting
    QSettings* m_setting;
    BL_OPTION m_option;

    // TrayIconMenu
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
