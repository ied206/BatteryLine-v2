#ifndef BATTERYSTATUS_H
#define BATTERYSTATUS_H

/*
 * Windows : Utilize SYSTEM_POWER_STATUS and GetSystemPowerStatus API
 * macOS   : Sadly, I don't have Apple device to implement and test, please contribute!
 * Linux   : Utilize UPower, using D-Bus System Bus
 */

#include <QObject>
#include <QList>
#include <cstdint>

#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

class PowerStatus
{
public:
<<<<<<< HEAD
    bool BatteryExist; // Is this system has battery?
    uint32_t BatteryLevel; // 0 - 100
    bool BatteryCharging; // Is battery is being charged?
    bool BatteryFull; // Is battery full?
    bool ACLineStatus; // Is this system is charging with AC Line?
=======
    bool m_BatteryExist; // Is this system has battery?
    int m_BatteryLevel; // 0 - 100
    bool m_BatteryCharging; // Is battery is being charged?
    bool m_BatteryFull; // Is battery full?
    bool m_ACLineStatus; // Is this system is charging with AC Line?
>>>>>>> master

    PowerStatus();
    ~PowerStatus();
    void Update();

private:
    QDBusInterface* m_CompositeBattery;
    QList<QDBusInterface*> m_LinePower;
};

#endif // BATTERYSTATUS_H
