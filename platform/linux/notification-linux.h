#ifndef NOTIFICATION_LINUX_H
#define NOTIFICATION_LINUX_H

#include <QObject>
#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

class Notification : public QObject
{
    Q_OBJECT

public:
    Notification();
    ~Notification();
    int SendNotification(const QString &summary, const QString &body);

private:
    void DeleteNotification(const uint id);
};

#endif // NOTIFICATION_LINUX_H

