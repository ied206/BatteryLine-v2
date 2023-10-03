#include "var.h"

#include "powernotify.h"
#include "../systemhelper.h"

#include <QDebug>

PowerNotify* PowerNotify::CreateInstance()
{
#ifdef Q_OS_WIN
    return new PowerNotifyWin();
#elif defined(Q_OS_LINUX)
    return new PowerNotifyLinux();
#endif
}

#ifdef Q_OS_WIN
PowerNotifyWin::PowerNotifyWin()
{

}

PowerNotifyWin::~PowerNotifyWin()
{

}

bool PowerNotifyWin::Register(void* handle)
{
    if (handle == nullptr)
    {
        SystemHelper::SystemError(QString("[%1] Cannot register PowerSettingNotification").arg(BL_PLATFORM));
        return false;
    }

    HWND hWnd = reinterpret_cast<HWND>(handle);
    this->m_hWnd = hWnd;

    // Register to Windows' Power Notification
    m_notPowerSrc = RegisterPowerSettingNotification(hWnd, &GUID_ACDC_POWER_SOURCE, DEVICE_NOTIFY_WINDOW_HANDLE);
    m_notBatPer = RegisterPowerSettingNotification(hWnd, &GUID_BATTERY_PERCENTAGE_REMAINING, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (m_notPowerSrc == nullptr || m_notBatPer == nullptr)
    {
        SystemHelper::SystemError(QString("[%1] Cannot register PowerSettingNotification").arg(BL_PLATFORM));
        return false;
    }

    return true;
}

bool PowerNotifyWin::Unregister()
{
    if (m_notPowerSrc == nullptr || m_notBatPer == nullptr)
    {
        SystemHelper::SystemError(QString("[%1] Cannot unregister PowerSettingNotification").arg(BL_PLATFORM));
        return false;
    }

    // Unregister from power notification
    BOOL result;
    result = UnregisterPowerSettingNotification(m_notBatPer);
    result &= UnregisterPowerSettingNotification(m_notPowerSrc);
    if (result == FALSE)
    {
        SystemHelper::SystemError(QString("[%1] Cannot unregister PowerSettingNotification").arg(BL_PLATFORM));
        return false;
    }

    return true;
}
#endif

#ifdef Q_OS_LINUX
PowerNotifyLinux::PowerNotifyLinux()
{
}

PowerNotifyLinux::~PowerNotifyLinux()
{

}

bool PowerNotifyLinux::Register(void* handle)
{
    (void)handle;

    if (!QDBusConnection::systemBus().isConnected())
    {
        SystemHelper::SystemError(QString("[%1] Cannot connect to D-Bus' system bus").arg(BL_PLATFORM));
        return false;
    }

    // Gather line power information
    QDBusInterface dBusUPower("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", QDBusConnection::systemBus());
    QDBusReply<QList<QDBusObjectPath>> dBusReply = dBusUPower.call("EnumerateDevices");
    if (dBusReply.isValid() == false)
    {
        SystemHelper::SystemError(QString("[%1] Cannot get list of power devices\nError = %2, %3")
                                      .arg(BL_PLATFORM, dBusReply.error().name(), dBusReply.error().message()));
        return false;
    }

    QList<QDBusObjectPath> devList = dBusReply.value();
    for (int i = 0; i < devList.count(); i++)
    {
        QDBusInterface dBusDeviceType("org.freedesktop.UPower", devList[i].path(), "org.freedesktop.UPower.Device", QDBusConnection::systemBus());
        switch (dBusDeviceType.property("Type").toUInt())
        {
        case 1: // Line Power
            m_LinePower.append(devList[i].path());
            break;
        case 2: // Battery
            break;
        default: // Ignore the others
            break;
        }
    }

    bool result;
    QDBusConnection dBusSystem = QDBusConnection::systemBus();
    result = dBusSystem.connect("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/DisplayDevice", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(BatteryInfoChanged(QString, QVariantMap, QStringList)));
    for (int i = 0; i < m_LinePower.count(); i++)
        result &= dBusSystem.connect("org.freedesktop.UPower", m_LinePower[i], "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(ACLineInfoChanged(QString, QVariantMap, QStringList)));
    if (result == false)
    {
        SystemHelper::SystemError(QString("[%1] Cannot register D-Bus System Bus").arg(BL_PLATFORM));
        return false;
    }

    return true;
}

bool PowerNotifyLinux::Unregister()
{
    // Unregister from power notification
    bool result;
    QDBusConnection dBusSystem = QDBusConnection::systemBus();
    result = dBusSystem.disconnect("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/DisplayDevice", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(BatteryInfoChanged(QString, QVariantMap, QStringList)));
    for (int i = 0; i < m_LinePower.count(); i++)
        result &= dBusSystem.connect("org.freedesktop.UPower", m_LinePower[i], "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(ACLineInfoChanged(QString, QVariantMap, QStringList)));
    if (result == false)
    {
        SystemHelper::SystemError(QString("[%1] Cannot unregister D-Bus System Bus").arg(BL_PLATFORM));
        return false;
    }

    return true;
}

// (QString interfaceName, QVariantMap changedProperties, QStringList invalidatedProperties)
void PowerNotifyLinux::BatteryInfoChanged(QString, QVariantMap changedProperties, QStringList)
{
    QVariant batteryLevel = changedProperties["Percentage"]; // double
    QVariant batteryState = changedProperties["State"] ;// uint

    if (batteryLevel.isValid() || batteryState.isValid()) // BatteryLevel or State has changed
        emit RedrawSignal();
}

// (QString interfaceName, QVariantMap changedProperties, QStringList invalidatedProperties)
void PowerNotifyLinux::ACLineInfoChanged(QString, QVariantMap changedProperties, QStringList)
{
    QVariant acLineOnline = changedProperties["Online"];
    if (acLineOnline.isValid()) // AC Adaptor has been plugged out or in
        emit RedrawSignal();
}
#endif
