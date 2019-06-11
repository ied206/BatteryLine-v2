#ifndef VAR_H
#define VAR_H

#include <QString>
#include <QDir>

#define BL_MAJOR_VER		2
#define BL_MINOR_VER		1
#define BL_ORG_NAME         QString("Joveler")
#define BL_ORG_DOMAIN       QString("joveler.kr")
#define BL_APP_NAME         QString("BatteryLine")
#define BL_WEB_BINARY		QString("https://ied206.github.io/BatteryLine")
#define BL_WEB_SOURCE		QString("https://github.com/ied206/BatteryLine")
#define BL_WEB_LICENSE      QString("https://github.com/ied206/BatteryLine/blob/master/LICENSE")

#define BL_ICON             QString(":/images/Cycle.png")

#define BL_LOCKFILE         QString(QDir::tempPath() + "/Joveler_BatteryLine_f527817e-41bd-43a8-86ca-7a20575297ec.lock")

#endif // VAR_H
