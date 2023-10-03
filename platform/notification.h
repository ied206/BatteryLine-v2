#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QObject>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_LINUX
#include <QtDBus>
#endif


class Notification : public QObject
{
    Q_OBJECT

public:
    Notification() { };
    virtual ~Notification() { };
    static Notification* CreateInstance();

    virtual bool Register(void* handle) = 0;
    virtual bool Unregister() = 0;

    virtual int SendNotification(const uint id, const QString &summary, const QString &body, const uint win_flags = 0, const uint win_infoFlags = 0) = 0;

protected:
    virtual void DeleteNotification(const uint id) = 0;
};

#ifdef Q_OS_WIN
class NotificationWin : public Notification
{
public:
    NotificationWin();
    virtual ~NotificationWin();

    virtual bool Register(void* handle);
    virtual bool Unregister();

    virtual int SendNotification(const uint id, const QString &summary, const QString &body, const uint win_flags = 0, const uint win_infoFlags = 0);

protected:
    HWND hWnd;
    HINSTANCE hInst;
    virtual void DeleteNotification(const uint id);
};
#endif

#ifdef Q_OS_LINUX
class NotificationLinux : public Notification
{
    Q_OBJECT

public:
    NotificationLinux();
    virtual ~NotificationLinux();

    virtual bool Register(void* handle);
    virtual bool Unregister();

    virtual int SendNotification(const uint id, const QString &summary, const QString &body, const uint win_flags = 0, const uint win_infoFlags = 0);

private:
    virtual void DeleteNotification(const uint id);
};
#endif

#endif // NOTIFICATION_H
