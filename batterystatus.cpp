#include "batterystatus.h"
#include "systemhelper.h"

#include <QSysInfo>
#include <QString>
#include <QDebug>
#include <QObject>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_LINUX
#include <QtDBus>

// Because of known issue of the Qt MOC, Qt's class cannot have #ifdef Q_OS_LINUX inside
static QDBusInterface* dBusDisplayDevice;
static QDBusInterface* dBusLinePowerAC;
#endif

BatteryStatus::BatteryStatus()
{
#ifdef Q_OS_WIN

#endif

#ifdef Q_OS_LINUX
    if (!QDBusConnection::systemBus().isConnected())
        SystemHelper::SystemError("[Linux] Cannot connect to D-Bus' system bus");

    dBusDisplayDevice = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/DisplayDevice", "org.freedesktop.UPower.Device", QDBusConnection::systemBus());
    dBusLinePowerAC = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/line_power_AC", "org.freedesktop.UPower.Device", QDBusConnection::systemBus());
#endif
}

BatteryStatus::~BatteryStatus()
{
#ifdef Q_OS_WIN

#endif

#ifdef Q_OS_LINUX
    delete dBusDisplayDevice;
    delete dBusLinePowerAC;

    dBusDisplayDevice = nullptr;
    dBusLinePowerAC = nullptr;
#endif
}

// Return true if success
void BatteryStatus::GetBatteryStatus()
{
#if defined(Q_OS_WIN)
    bool success = true;
    SYSTEM_POWER_STATUS batStat;
    if (!GetSystemPowerStatus(&batStat))
        success = false; // Error
    else if (batStat.BatteryFlag == 255
         || batStat.ACLineStatus == 255 // Check Error
         || (!(batStat.BatteryFlag & 128) && batStat.BatteryLifePercent == 255)) // Has Battery but BatteryLifePercent == 255 -> error
         success = false;

    if (success)
    {
        this->m_BatteryExist = (batStat.BatteryFlag & 128) ? false : true;
        this->m_BatteryLevel = batStat.BatteryLifePercent;
        this->m_BatteryCharging = (batStat.BatteryFlag & 8) ? true : false;
        this->m_BatteryFull = (batStat.ACLineStatus && (batStat.BatteryFlag & 8) == false) ? true : false; // Not Charging, because battery is full
        this->m_ACLineStatus = batStat.ACLineStatus ? true : false;
    }
    else
        SystemHelper::SystemError(QObject::tr("[Windows] Cannot get power state"));

#elif defined(Q_OS_LINUX)
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

#endif

    qDebug() << "[BatteryStatus]";
    qDebug() << "BatteryExist    : " << m_BatteryExist;
    qDebug() << "BatteryLevel    : " << m_BatteryLevel;
    qDebug() << "BatteryCharging : " << m_BatteryCharging;
    qDebug() << "BatteryFull     : " << m_BatteryFull;
    qDebug() << "ACLineStatus    : " << m_ACLineStatus << "\n";
}

