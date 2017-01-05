#include "var.h"
#include "resource.h"

#include "notification-win.h"
#include "../../systemhelper.h"

#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <strsafe.h>
#endif

Notification::Notification(HWND hWnd)
{
    this->hWnd = hWnd;
    this->hInst = (HINSTANCE) GetWindowLongPtrW(hWnd, GWLP_HINSTANCE);
}

Notification::~Notification()
{

}

void Notification::SendNotification(const uint id, const QString &summary, const QString &body, const uint win_flags, const uint win_infoFlags)
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
}

void Notification::DeleteNotification(const uint id)
{
    NOTIFYICONDATAW nid;

    nid.hWnd = hWnd;
    nid.uID = id;

    Shell_NotifyIconW(NIM_DELETE, &nid);
}
