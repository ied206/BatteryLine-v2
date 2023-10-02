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
    return true;
}

bool PowerStatusWin::Unregister()
{
    return true;
}

// Return true if success
void PowerStatusWin::Update()
{
    bool success = true;
    SYSTEM_POWER_STATUS batStat;
    if (!GetSystemPowerStatus(&batStat))
        success = false; // Error
    else if (batStat.BatteryFlag == 255 ||
             batStat.ACLineStatus == 255 || // Check Error
             (!(batStat.BatteryFlag & 128) && batStat.BatteryLifePercent == 255)) // Has Battery but BatteryLifePercent == 255 -> error
        success = false;

    if (success)
    {
        this->m_BatteryExist = (batStat.BatteryFlag & 128) ? false : true;
        this->m_BatteryLevel = batStat.BatteryLifePercent;
        this->m_BatteryCharging = (batStat.BatteryFlag & 8) ? true : false;
        this->m_BatteryFull = batStat.ACLineStatus && (batStat.BatteryFlag & 8) == false; // Not Charging, because battery is full
        this->m_ACLineStatus = batStat.ACLineStatus ? true : false;
    }
    else
    {
        SystemHelper::SystemError(QObject::tr("[Windows] Cannot retrieve power information.\n"));
    }

#ifdef _DEBUG
    qDebug() << "[BatteryStatus]";
    qDebug() << "BatteryExist    : " << m_BatteryExist;
    qDebug() << "BatteryLevel    : " << m_BatteryLevel;
    qDebug() << "BatteryCharging : " << m_BatteryCharging;
    qDebug() << "BatteryFull     : " << m_BatteryFull;
    qDebug() << "ACLineStatus    : " << m_ACLineStatus << "\n";
#endif
}
#endif

#ifdef Q_OS_LINUX
PowerStatusLinux::PowerStatusLinux()
{
    if (!QDBusConnection::systemBus().isConnected())
        SystemHelper::SystemError(QString("[%1] Cannot connect to D-Bus' system bus").arg(BL_PLATFORM));

    // Composite Battery which is virtualized
    m_CompositeBattery = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/DisplayDevice", "org.freedesktop.UPower.Device", QDBusConnection::systemBus());

    // Gather line power information
    QDBusInterface dBusUPower("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", QDBusConnection::systemBus());
    QDBusReply<QList<QDBusObjectPath>> dBusReply = dBusUPower.call("EnumerateDevices");
    if (dBusReply.isValid() == false)
    {
        SystemHelper::SystemError(QString("[%1] Cannot get list of power devices\nError = %2, %3")
                                      .arg(BL_PLATFORM)
                                      .arg(dBusReply.error().name())
                                      .arg(dBusReply.error().message()));
    }

    QList<QDBusObjectPath> devList = dBusReply.value();
    for (int i = 0; i < devList.count(); i++)
    {
        QDBusInterface dBusDeviceType("org.freedesktop.UPower", devList[i].path(), "org.freedesktop.UPower.Device", QDBusConnection::systemBus());
        QVariant type = dBusDeviceType.property("Type");
        switch(type.toUInt())
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

    Update();
}

PowerStatusLinux::~PowerStatusLinux()
{
    delete m_CompositeBattery;
    while (m_LinePower.isEmpty() == false)
    {
        delete m_LinePower[0];
        m_LinePower.removeFirst();
    }

    m_CompositeBattery = nullptr;
    m_LinePower.clear();
}

// Return true if success
void PowerStatusLinux::Update()
{
    // https://upower.freedesktop.org/docs/Device.html
    // line power status
    QList<QVariant> acLineStatus;
    for (int i = 0; i < m_LinePower.count(); i++)
        acLineStatus.append(m_LinePower[i]->property("Online")); // bool
    this->m_ACLineStatus = true;
    for (int i = 0; i < acLineStatus.count(); i++)
        this->m_ACLineStatus &= acLineStatus[i].toBool();

    // battery status
    QVariant batteryLevel = m_CompositeBattery->property("Percentage"); // double
    QVariant batteryExist = m_CompositeBattery->property("IsPresent");  // bool
    QVariant batteryState = m_CompositeBattery->property("State");      // uint
    this->m_BatteryExist = batteryExist.toBool();
    this->m_BatteryLevel = batteryLevel.toInt();
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
    case 0: // Unknown OR Battery does not exist
        if (this->m_BatteryExist) // Error when battery DOES exist
            SystemHelper::SystemError(QString("[%1] Battery state unclear").arg(BL_PLATFORM));
        break;
    default:
        SystemHelper::SystemError(QString("[%1] Battery state unclear").arg(BL_PLATFORM));
        break;
    }

#ifdef _DEBUG
    qDebug() << "[BatteryStatus]";
    qDebug() << "BatteryExist    : " << m_BatteryExist;
    qDebug() << "BatteryLevel    : " << m_BatteryLevel;
    qDebug() << "BatteryCharging : " << m_BatteryCharging;
    qDebug() << "BatteryFull     : " << m_BatteryFull;
    qDebug() << "ACLineStatus    : " << m_ACLineStatus << "\n";
#endif
}
#endif
