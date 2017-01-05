#include "var.h"

#include "notification-linux.h"
#include "../../systemhelper.h"

Notification::Notification()
{
    if (!QDBusConnection::sessionBus().isConnected())
        SystemHelper::SystemError("[Linux] Cannot connect to D-Bus Session bus");
}

Notification::~Notification()
{
}

int Notification::SendNotification(const QString &summary, const QString &body)
{
    // https://developer.gnome.org/notification-spec/
    // http://www.galago-project.org/specs/notification/0.9/x408.html
    // $ qdbus org.freedesktop.Notifications /org/freedesktop/Notifications
    QDBusConnection dBusNotify = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "org.freedesktop.Notifications");
    QDBusMessage dBusReqeust = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify");

    QList<QVariant> dBusArg;
    dBusArg.append("BatteryLine"); // app_name
    dBusArg.append(0); // replaces_id
    dBusArg.append(""); // app_icon
    dBusArg.append(summary); // summary
    dBusArg.append(body); // body
    dBusArg.append(QStringList()); // actions
    dBusArg.append(QVariantMap()); // hints
    dBusArg.append(-1); // expire_timeout
    dBusReqeust.setArguments(dBusArg);
    QDBusMessage dBusReply = dBusNotify.call(dBusReqeust, QDBus::Block);
    if (dBusReply.type() == QDBusMessage::ErrorMessage)
        SystemHelper::SystemError("[Linux] Cannot send notification through D-Bus");

    QList<QVariant> dBusReturn = dBusReply.arguments();
    return dBusReturn.first().toUInt();
}

void Notification::DeleteNotification(const uint id)
{
    QDBusConnection dBusNotify = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "org.freedesktop.Notifications");
    QDBusMessage dBusReqeust = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "CloseNotification");

    QList<QVariant> dBusArg;
    dBusArg.append(id); // replaces_id
    dBusReqeust.setArguments(dBusArg);
    QDBusMessage dBusReply = dBusNotify.call(dBusReqeust, QDBus::Block);
    if (dBusReply.type() == QDBusMessage::ErrorMessage)
        SystemHelper::SystemError("[Linux] Cannot delete notification through D-Bus");
}
