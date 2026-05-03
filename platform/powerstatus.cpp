#include "var.h"
#include "powerstatus.h"
#include "../systemhelper.h"

#include <QSysInfo>
#include <QString>
#include <QDebug>
#include <QObject>

#ifdef Q_OS_WIN
#include <Windows.h>
#elif defined(Q_OS_LINUX)
#include <QtDBus>
#endif

PowerStatus* PowerStatus::CreateInstance()
{
#ifdef Q_OS_WIN
    return new PowerStatusWin();
#elif defined(Q_OS_LINUX)
    return new PowerStatusLinux();
#endif
}

#ifdef Q_OS_WIN
PowerStatusWin::PowerStatusWin()
{

}

PowerStatusWin::~PowerStatusWin()
{

}

bool PowerStatusWin::Register(void* handle)
{
    (void)handle;
    return Update();
}

bool PowerStatusWin::Unregister()
{
    return true;
}

bool PowerStatusWin::Update()
{
    SYSTEM_POWER_STATUS batStat;
    if (!GetSystemPowerStatus(&batStat))
    {
        qWarning() << "[Windows] Cannot retrieve power information.";
        return false;
    }

    if (batStat.BatteryFlag == 255 ||
        batStat.ACLineStatus == 255 ||
        (!(batStat.BatteryFlag & 128) && batStat.BatteryLifePercent == 255))
    {
        qWarning() << "[Windows] Power information is temporarily unavailable.";
        return false;
    }

    bool batteryExist = (batStat.BatteryFlag & 128) ? false : true;
    if (!batteryExist && this->m_BatteryExist)
    {
        qWarning() << "[Windows] Battery information is temporarily unavailable.";
        return false;
    }

    this->m_BatteryExist = batteryExist;
    this->m_BatteryLevel = batStat.BatteryLifePercent;
    this->m_BatteryCharging = (batStat.BatteryFlag & 8) ? true : false;
    this->m_ACLineStatus = (batStat.ACLineStatus == 1) ? true : false;
    this->m_BatteryFull = this->m_ACLineStatus && (batStat.BatteryFlag & 8) == false; // Not Charging, because battery is full

#ifdef _DEBUG
    qDebug().noquote() << "[BatteryStatus]";
    qDebug().noquote() << "BatteryExist    : " << m_BatteryExist;
    qDebug().noquote() << "BatteryLevel    : " << m_BatteryLevel;
    qDebug().noquote() << "BatteryCharging : " << m_BatteryCharging;
    qDebug().noquote() << "BatteryFull     : " << m_BatteryFull;
    qDebug().noquote() << "ACLineStatus    : " << m_ACLineStatus << "\n";
#endif

    return true;
}
#endif

#ifdef Q_OS_LINUX
PowerStatusLinux::PowerStatusLinux() :
    m_CompositeBattery(nullptr)
{

}

PowerStatusLinux::~PowerStatusLinux()
{

}

bool PowerStatusLinux::Register(void* handle)
{
    (void)handle;

    if (!QDBusConnection::systemBus().isConnected())
    {
        SystemHelper::SystemError(QString("[%1] Cannot connect to D-Bus' system bus").arg(SystemHelper::OSName()));
        return false;
    }

    // Composite Battery which is virtualized
    m_CompositeBattery = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/DisplayDevice", "org.freedesktop.UPower.Device", QDBusConnection::systemBus());

    // Gather line power information
    QDBusInterface dBusUPower("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", QDBusConnection::systemBus());
    QDBusReply<QList<QDBusObjectPath>> dBusReply = dBusUPower.call("EnumerateDevices");
    if (dBusReply.isValid() == false)
    {
        SystemHelper::SystemError(QString("[%1] Cannot get list of power devices\nError = %2, %3")
                                      .arg(SystemHelper::OSName(), dBusReply.error().name(), dBusReply.error().message()));
        return false;
    }

    QList<QDBusObjectPath> devList = dBusReply.value();
    for (int i = 0; i < devList.count(); i++)
    {
        QDBusInterface dBusDeviceType("org.freedesktop.UPower", devList[i].path(), "org.freedesktop.UPower.Device", QDBusConnection::systemBus());
        QVariant type = dBusDeviceType.property("Type");
        switch (type.toUInt())
        {
        case 1: // Line Power
            m_LinePower.append(new QDBusInterface("org.freedesktop.UPower", devList[i].path(), "org.freedesktop.UPower.Device", QDBusConnection::systemBus()));
            break;
        case 2: // Battery
            break;
        default: // Ignore the others
            break;
        }
    }

    return Update();
}

bool PowerStatusLinux::Unregister()
{
    delete m_CompositeBattery;
    while (m_LinePower.isEmpty() == false)
    {
        delete m_LinePower[0];
        m_LinePower.removeFirst();
    }

    m_CompositeBattery = nullptr;
    m_LinePower.clear();

    return true;
}

bool PowerStatusLinux::Update()
{
    if (m_CompositeBattery == nullptr)
        return false;

    // https://upower.freedesktop.org/docs/Device.html
    // line power status
    this->m_ACLineStatus = false;
    for (int i = 0; i < m_LinePower.count(); i++)
    {
        QVariant online = m_LinePower[i]->property("Online"); // bool
        if (online.isValid())
            this->m_ACLineStatus |= online.toBool();
    }

    // battery status
    QVariant batteryLevel = m_CompositeBattery->property("Percentage"); // double
    QVariant batteryExist = m_CompositeBattery->property("IsPresent");  // bool
    QVariant batteryState = m_CompositeBattery->property("State");      // uint
    if (!batteryLevel.isValid() || !batteryExist.isValid() || !batteryState.isValid())
    {
        qWarning() << "[Linux] Battery information is temporarily unavailable.";
        return false;
    }

    this->m_BatteryExist = batteryExist.toBool();
    this->m_BatteryLevel = qBound(0, batteryLevel.toInt(), 100);
    switch (batteryState.toUInt())
    {
    case 1: // Charging
    case 5: // Pending charge
        this->m_ACLineStatus = true;
        this->m_BatteryCharging = true;
        this->m_BatteryFull = false;
        break;
    case 2: // Discharging
    case 3: // Empty
    case 6: // Pending discharge
        this->m_BatteryCharging = false;
        // In VMware, ACLineStatus is always true
        if (this->m_ACLineStatus == true && 90 <= this->m_BatteryLevel)
        {
            this->m_BatteryFull = true;
        }
        else
        {
            this->m_ACLineStatus = false;
            this->m_BatteryFull = false;
        }
        /*
        if (this->m_ACLineStatus == true)
            this->m_BatteryFull = true;
        else
            this->m_BatteryFull = false;
        */
        break;
    case 4: // Fully charged
        this->m_ACLineStatus = true;
        this->m_BatteryCharging = false;
        this->m_BatteryFull = true;
        break;
    case 0: // Unknown OR Battery does not exist
        if (this->m_BatteryExist) // Error when battery DOES exist
        {
            qWarning() << "[Linux] Battery state is temporarily unclear.";
            return false;
        }
        break;
    default:
        qWarning() << "[Linux] Battery state is unclear:" << batteryState;
        return false;
    }

#ifdef _DEBUG
    qDebug().noquote() << "[BatteryStatus]";
    qDebug().noquote() << "BatteryExist    : " << m_BatteryExist;
    qDebug().noquote() << "BatteryLevel    : " << m_BatteryLevel;
    qDebug().noquote() << "BatteryCharging : " << m_BatteryCharging;
    qDebug().noquote() << "BatteryFull     : " << m_BatteryFull;
    qDebug().noquote() << "ACLineStatus    : " << m_ACLineStatus << "\n";
#endif

    return true;
}
#endif
