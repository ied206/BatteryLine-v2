#include "batterystatus-win.h"
#include "../../systemhelper.h"

#include <QSysInfo>
#include <QString>
#include <QDebug>
#include <QObject>

#ifdef Q_OS_WIN
#include <windows.h>
#endif


BatteryStatus::BatteryStatus()
{
    GetBatteryStatus();
}

BatteryStatus::~BatteryStatus()
{

}

// Return true if success
void BatteryStatus::GetBatteryStatus()
{
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

    qDebug() << "[BatteryStatus]";
    qDebug() << "BatteryExist    : " << m_BatteryExist;
    qDebug() << "BatteryLevel    : " << m_BatteryLevel;
    qDebug() << "BatteryCharging : " << m_BatteryCharging;
    qDebug() << "BatteryFull     : " << m_BatteryFull;
    qDebug() << "ACLineStatus    : " << m_ACLineStatus << "\n";
}

