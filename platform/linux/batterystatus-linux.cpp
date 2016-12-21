#include "batterystatus-linux.h"
#include "../../systemhelper.h"

#include <QSysInfo>
#include <QString>
#include <QDebug>
#include <QObject>

#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

BatteryStatus::BatteryStatus()
{
    if (!QDBusConnection::systemBus().isConnected())
        SystemHelper::SystemError("[Linux] Cannot connect to D-Bus' system bus");

    dBusDisplayDevice = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/DisplayDevice", "org.freedesktop.UPower.Device", QDBusConnection::systemBus());
    dBusLinePowerAC = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/line_power_AC", "org.freedesktop.UPower.Device", QDBusConnection::systemBus());

    GetBatteryStatus();
}

BatteryStatus::~BatteryStatus()
{
    delete dBusDisplayDevice;
    delete dBusLinePowerAC;

    dBusDisplayDevice = nullptr;
    dBusLinePowerAC = nullptr;
}

// Return true if success
void BatteryStatus::GetBatteryStatus()
{
    // https://upower.freedesktop.org/docs/Device.html
    QVariant batteryLevel = dBusDisplayDevice->property("Percentage"); // double
    QVariant batteryExist = dBusDisplayDevice->property("IsPresent");  // bool
    QVariant batteryState = dBusDisplayDevice->property("State");      // uint
    QVariant acLineStatus = dBusLinePowerAC->property("Online");       // bool

    this->m_BatteryExist = batteryExist.toBool();
    this->m_BatteryLevel = batteryLevel.toInt();
    this->m_ACLineStatus = acLineStatus.toBool();
    switch (batteryState.toInt())
    {
    case 1: // Charging
    case 5: // Pending charge
        this->m_BatteryCharging = true;
        this->m_BatteryFull = false;
        break;
    case 2: // Discharging
    case 3: // Empty
    case 6: // Pending discharge
        this->m_BatteryCharging = false;
        if (this->m_ACLineStatus == true)
            this->m_BatteryFull = true;
        else
            this->m_BatteryFull = false;
        break;
    case 4: // Fully charged
        this->m_BatteryCharging = false;
        this->m_BatteryFull = true;
        break;
    case 0: // Unknown
    default:
        SystemHelper::SystemError(QObject::tr("[Linux] Power state unclear"));
        break;
    }

    qDebug() << "[BatteryStatus]";
    qDebug() << "BatteryExist    : " << m_BatteryExist;
    qDebug() << "BatteryLevel    : " << m_BatteryLevel;
    qDebug() << "BatteryCharging : " << m_BatteryCharging;
    qDebug() << "BatteryFull     : " << m_BatteryFull;
    qDebug() << "ACLineStatus    : " << m_ACLineStatus << "\n";
}

