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
        SystemHelper::SystemError("[Linux] Cannot connect to D-Bus' system bus");

    dBusDisplayDevice = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/DisplayDevice", "org.freedesktop.UPower.Device", QDBusConnection::systemBus());
    dBusLinePowerAC = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower/devices/line_power_AC", "org.freedesktop.UPower.Device", QDBusConnection::systemBus());

    Update();
}

PowerStatus::~PowerStatus()
{
    delete dBusDisplayDevice;
    delete dBusLinePowerAC;

    dBusDisplayDevice = nullptr;
    dBusLinePowerAC = nullptr;
}

// Return true if success
void PowerStatus::Update()
{
    // https://upower.freedesktop.org/docs/Device.html
    QVariant batteryLevel = dBusDisplayDevice->property("Percentage"); // double
    QVariant batteryExist = dBusDisplayDevice->property("IsPresent");  // bool
    QVariant batteryState = dBusDisplayDevice->property("State");      // uint
    QVariant acLineStatus = dBusLinePowerAC->property("Online");       // bool

    this->BatteryExist = batteryExist.toBool();
    this->BatteryLevel = batteryLevel.toInt();
    this->ACLineStatus = acLineStatus.toBool();
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
    case 0: // Unknown
    default:
        SystemHelper::SystemError(QObject::tr("[Linux] Power state unclear"));
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

