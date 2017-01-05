#ifndef NOTIFICATION_WIN_H
#define NOTIFICATION_WIN_H

#include <QObject>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

class Notification : public QObject
{
    Q_OBJECT

public:
    Notification(HWND hWnd);
    ~Notification();
    void SendNotification(const uint id, const QString &summary, const QString &body, const uint win_flags = 0, const uint win_infoFlags = 0);

private:
    HWND hWnd;
    HINSTANCE hInst;
    void DeleteNotification(const uint id);
};

#endif // NOTIFICATION_WIN_H

