#include "var.h"
#include "resource.h"

#include "notification.h"
#include "../systemhelper.h"

#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <strsafe.h>
#endif

Notification* Notification::CreateInstance()
{
#ifdef Q_OS_WIN
    return new NotificationWin();
#elif defined(Q_OS_LINUX)
    return new NotificationLinux();
#endif
}

#ifdef Q_OS_WIN
NotificationWin::NotificationWin()
{

}

NotificationWin::~NotificationWin()
{

}

bool NotificationWin::Register(void* handle)
{
    HWND hWnd = reinterpret_cast<HWND>(handle);

    this->hWnd = hWnd;
    this->hInst = (HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE);

    return true;
}

bool NotificationWin::Unregister()
{
    return true;
}

int NotificationWin::SendNotification(const uint id, const QString &summary, const QString &body, const uint win_flags, const uint win_infoFlags)
{
    NOTIFYICONDATAW nid;
    ZeroMemory(&nid, sizeof(NOTIFYICONDATAW));

    // Notification Icon
    nid.cbSize 		= sizeof(NOTIFYICONDATAW);
    nid.hWnd 		= hWnd;
    nid.uID 		= id;
    nid.uFlags 		= NIF_ICON | win_flags;
    nid.dwInfoFlags = NIIF_USER | win_infoFlags;
    nid.uCallbackMessage = WM_NULL; // Don't throw an message

#ifdef _MSC_VER
    LoadIconMetric(hInst, MAKEINTRESOURCEW(IDI_MAINICON), LIM_SMALL, &(nid.hIcon)); // Load the icon for high DPI. However, MinGW-w64 cannot link this function...
#else
    nid.hIcon 		= (HICON) LoadImageW(hInst, MAKEINTRESOURCEW(IDI_MAINICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
#endif
    StringCchCopyW(nid.szTip, ARRAYSIZE(nid.szTip), (STRSAFE_LPCWSTR) summary.utf16());
    StringCchCopyW(nid.szInfo, ARRAYSIZE(nid.szInfo), (STRSAFE_LPCWSTR) body.utf16());
    nid.uVersion 	= NOTIFYICON_VERSION_4;

    Shell_NotifyIconW(NIM_SETVERSION, &nid);
    Shell_NotifyIconW(NIM_ADD, &nid);

    DeleteNotification(id);
    return 0;
}

void NotificationWin::DeleteNotification(const uint id)
{
    NOTIFYICONDATAW nid;

    nid.hWnd = hWnd;
    nid.uID = id;

    Shell_NotifyIconW(NIM_DELETE, &nid);
}
#endif

#ifdef Q_OS_LINUX
NotificationLinux::NotificationLinux()
{

}

NotificationLinux::~NotificationLinux()
{
}

bool NotificationLinux::Register(void* handle)
{
    (void)handle;

    if (!QDBusConnection::sessionBus().isConnected())
    {
        SystemHelper::SystemError("[Linux] Cannot connect to D-Bus Session bus");
        return false;
    }

    return true;
}

bool NotificationLinux::Unregister()
{
    return true;
}

int NotificationLinux::SendNotification(const uint id, const QString &summary, const QString &body, const uint win_flags, const uint win_infoFlags)
{
    (void)id;
    (void)win_flags;
    (void)win_infoFlags;
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

void NotificationLinux::DeleteNotification(const uint id)
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
#endif
