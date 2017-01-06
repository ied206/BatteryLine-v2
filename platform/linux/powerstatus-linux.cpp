#include "var.h"
#include "powerstatus-linux.h"
#include "../../systemhelper.h"

#include <QSysInfo>
#include <QString>
#include <QDebug>
#include <QObject>

#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

PowerStatus::PowerStatus()
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

PowerStatus::~PowerStatus()
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
void PowerStatus::Update()
{
    // https://upower.freedesktop.org/docs/Device.html
    // line power status
    QList<QVariant> acLineStatus;
    for (int i = 0; i < m_LinePower.count(); i++)
        acLineStatus.append(m_LinePower[i]->property("Online")); // bool
    this->ACLineStatus = true;
    for (int i = 0; i < acLineStatus.count(); i++)
        this->ACLineStatus &= acLineStatus[i].toBool();

    // battery status
    QVariant batteryLevel = m_CompositeBattery->property("Percentage"); // double
    QVariant batteryExist = m_CompositeBattery->property("IsPresent");  // bool
    QVariant batteryState = m_CompositeBattery->property("State");      // uint
    this->BatteryExist = batteryExist.toBool();
    this->BatteryLevel = batteryLevel.toInt();
    switch (batteryState.toInt())
    {
    case 1: // Charging
    case 5: // Pending charge
        this->BatteryCharging = true;
        this->BatteryFull = false;
        break;
    case 2: // Discharging
    case 3: // Empty
    case 6: // Pending discharge
        this->BatteryCharging = false;
        if (this->ACLineStatus == true)
            this->BatteryFull = true;
        else
            this->BatteryFull = false;
        break;
    case 4: // Fully charged
        this->BatteryCharging = false;
        this->BatteryFull = true;
        break;
    case 0: // Unknown OR Battery does not exist
        if (this->BatteryExist) // Error when battery DOES exist
            SystemHelper::SystemError(QString("[%1] Battery state unclear").arg(BL_PLATFORM));
        break;
    default:
        SystemHelper::SystemError(QString("[%1] Battery state unclear").arg(BL_PLATFORM));
        break;
    }

#ifdef _DEBUG
    qDebug() << "[BatteryStatus]";
    qDebug() << "BatteryExist    : " << BatteryExist;
    qDebug() << "BatteryLevel    : " << BatteryLevel;
    qDebug() << "BatteryCharging : " << BatteryCharging;
    qDebug() << "BatteryFull     : " << BatteryFull;
    qDebug() << "ACLineStatus    : " << ACLineStatus << "\n";
#endif
}

