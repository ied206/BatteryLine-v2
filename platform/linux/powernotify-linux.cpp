#include "var.h"

#include "powernotify-linux.h"
#include "../../systemhelper.h"

#include <QDebug>

PowerNotify::PowerNotify()
{
    if (!QDBusConnection::systemBus().isConnected())
        SystemHelper::SystemError("[Linux] Cannot connect to D-Bus system bus");

    bool result;
    QDBusConnection dBusSystem = QDBusConnection::systemBus();
    result = dBusSystem.connect("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/DisplayDevice", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(BatteryInfoChanged(QString, QVariantMap, QStringList)));
    result &= dBusSystem.connect("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/line_power_AC", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(ACLineInfoChanged(QString, QVariantMap, QStringList)));
    if (result == false)
        SystemHelper::SystemError(tr("[Linux] Cannot register D-Bus System Bus"));
}

PowerNotify::~PowerNotify()
{
    // Unregister from power notification
    bool result;
    QDBusConnection dBusSystem = QDBusConnection::systemBus();
    result = dBusSystem.disconnect("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/DisplayDevice", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(BatteryInfoChanged(QString, QVariantMap, QStringList)));
    result &= dBusSystem.disconnect("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/line_power_AC", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(ACLineInfoChanged(QString, QVariantMap, QStringList)));
    if (result == false)
        SystemHelper::SystemError(tr("[Linux] Cannot unregister D-Bus System Bus"));
}

// (QString interfaceName, QVariantMap changedProperties, QStringList invalidatedProperties)
void PowerNotify::BatteryInfoChanged(QString, QVariantMap changedProperties, QStringList)
{
    QVariant batteryLevel = changedProperties["Percentage"]; // double
    QVariant batteryState = changedProperties["State"] ;// uint

    if (batteryLevel.isValid() || batteryState.isValid()) // BatteryLevel or State has changed
        emit RedrawSignal();
}

// (QString interfaceName, QVariantMap changedProperties, QStringList invalidatedProperties)
void PowerNotify::ACLineInfoChanged(QString, QVariantMap changedProperties, QStringList)
{
    QVariant acLineOnline = changedProperties["Online"];
    if (acLineOnline.isValid())  // AC Adaptor has been plugged out or in
        emit RedrawSignal();
}

