#ifndef POWERSTATUS_H
#define POWERSTATUS_H

#include <cstdint>
#include <QList>
#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

/*
 * Windows : Utilize SYSTEM_POWER_STATUS and GetSystemPowerStatus API
 * macOS   : TODO (Is top-level window in macOS possible?)
 * Linux   : Utilize UPower, using D-Bus System Bus
 */

class PowerStatus
{
public:
    // Does this system have a battery?
    bool m_BatteryExist;
    // Battery level in percent, 0 - 100
    int m_BatteryLevel;
    // Is battery being charged?
    bool m_BatteryCharging;
    // Is battery full?
    bool m_BatteryFull;
    // Is this system charging with AC Line?
    bool m_ACLineStatus;

    PowerStatus() :
        m_BatteryExist(false),
        m_BatteryLevel(0),
        m_BatteryCharging(false),
        m_BatteryFull(false),
        m_ACLineStatus(false)
    { };
    virtual ~PowerStatus() { };
    static PowerStatus* CreateInstance();

    virtual bool Register(void* handle) = 0;
    virtual bool Unregister() = 0;

    virtual bool Update() = 0;
};

#ifdef Q_OS_WIN
class PowerStatusWin : public PowerStatus
{
public:
    PowerStatusWin();
    virtual ~PowerStatusWin();

    virtual bool Register(void* handle);
    virtual bool Unregister();

    virtual bool Update();
};
#endif

#ifdef Q_OS_LINUX
class PowerStatusLinux : public PowerStatus
{
public:
    PowerStatusLinux();
    virtual ~PowerStatusLinux();

    virtual bool Register(void* handle);
    virtual bool Unregister();

    virtual bool Update();

protected:
    QDBusInterface* m_CompositeBattery;
    QList<QDBusInterface*> m_LinePower;
};
#endif

#endif // POWERSTATUS_H
