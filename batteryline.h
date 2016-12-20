#ifndef BATTERYLINE_H
#define BATTERYLINE_H

// sudo apt install libgl-dev

#include "batterystatus.h"

#include <QWidget>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QString>

#ifdef Q_OS_WIN
#include "platform/win/powernotify-win.h"
#endif
#ifdef Q_OS_LINUX
#include "platform/linux/powernotify-linux.h"
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
    void TrayMenuExit();
    void TrayMenuPrintBanner();
    void RedrawLine();

private:
    Ui::BatteryLine *ui;
    void RegisterPowerNotification();
    void UnregisterPowerNotification();
    void SetWindowSizePos();
    void SetColor();
    void CreateTrayIcon();

    PowerNotify* m_powerNotify;
    BatteryStatus* m_batStat;
    QMenu* trayIconMenu;
    QSystemTrayIcon* trayIcon;
    QAction* exitAct;
    QAction* printBannerAct;
};

#endif // BATTERYLINE_H
