#ifndef BATTERYSTATUS_H
#define BATTERYSTATUS_H

/*
 * Windows : Utilize SYSTEM_POWER_STATUS and GetSystemPowerStatus API
 * macOS   : Sadly, I don't have Apple device to implement and test
 * Linux   : Utilize UPower, using D-Bus' System Bus
 */

#include <QObject>
#include <cstdint>

#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

class PowerStatus
{
public:
    bool m_BatteryExist; // Is this system has battery?
    uint32_t m_BatteryLevel; // 0 - 100
    bool m_BatteryCharging; // Is battery is being charged?
    bool m_BatteryFull; // Is battery full?
    bool m_ACLineStatus; // Is this system is charging with AC Line?

    PowerStatus();
    ~PowerStatus();
    void Update();

private:
    QDBusInterface* dBusDisplayDevice;
    QDBusInterface* dBusLinePowerAC;
};

#endif // BATTERYSTATUS_H
