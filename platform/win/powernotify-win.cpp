#include "var.h"

#include "powernotify-win.h"
#include "../../systemhelper.h"

#include <QDebug>

PowerNotify::PowerNotify(HWND hWnd)
{
    this->hWnd = hWnd;

    // Register to Windows' Power Notification
    m_notPowerSrc = RegisterPowerSettingNotification(hWnd, &GUID_ACDC_POWER_SOURCE, DEVICE_NOTIFY_WINDOW_HANDLE);
    m_notBatPer = RegisterPowerSettingNotification(hWnd, &GUID_BATTERY_PERCENTAGE_REMAINING, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (m_notPowerSrc == NULL || m_notBatPer == NULL)
        SystemHelper::SystemError(tr("[Windows] Cannot register PowerSettingNotification"));
}

PowerNotify::~PowerNotify()
{
    // Unregister from power notification
    BOOL result;
    result = UnregisterPowerSettingNotification(m_notBatPer);
    result &= UnregisterPowerSettingNotification(m_notPowerSrc);
    if (result == FALSE)
        SystemHelper::SystemError(tr("[Windows] Cannot unregister PowerSettingNotification"));
}
