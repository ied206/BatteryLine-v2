#ifndef VAR_H
#define VAR_H

#include "resource.h"
#ifdef _DEBUG
#include "systemhelper.h"
#endif

#include <QString>
#include <QDir>
#include <QVersionNumber>

#define BL_VER_INST         QVersionNumber(2, 1)
#ifdef _DEBUG // Debug - Use __DATE__
#define BL_REL_DATE         (QString("%1%2%3").arg(SystemHelper::CompileYear(), 4, 10, QChar('0')).arg(SystemHelper::CompileMonth(), 2, 10, QChar('0')).arg(SystemHelper::CompileDay(), 2, 10, QChar('0')))
#else // Release - Use internal date
#define BL_REL_DATE         QString("20231023")
#endif
#define BL_ORG_NAME         QString(RES_COMPANYNAME_STR)
#define BL_ORG_DOMAIN       QString(RES_COMPANYDOMAIN_STR)
#define BL_APP_NAME         QString(RES_FILEDESCRIPTION_STR)
#define BL_WEB_SOURCE		QString("https://github.com/ied206/BatteryLine-v2")

#define BL_GUID             QString("f527817e-41bd-43a8-86ca-7a20575297ec")
#define BL_LOCKID           QString(QDir::tempPath() + "/Joveler_BatteryLine_" + BL_GUID)
#define BL_ICON             QString(":/images/Cycle.png")

// For Windows Platform
#define BL_NOTIFY_APP_ON    1
#define BL_NOTIFY_APP_OFF   2

#endif // VAR_H
