#include "var.h"
#include "batteryline.h"
#include "systemhelper.h"
#include <QApplication>

#include <QString>
#include <QLockFile>
#include <QDir>
#include <QMessageBox>

#ifdef Q_OS_LINUX
#include <QStyle>
#include <QStyleFactory>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef Q_OS_LINUX
    // Qt 5.7 does not have gtk theme anymore, so in that case, fallback to Fusion theme
    QStringList list = QStyleFactory::keys();
    if (list.contains("gtk", Qt::CaseInsensitive))
        QApplication::setStyle("gtk");
    else if (list.contains("Fusion", Qt::CaseInsensitive))
        QApplication::setStyle("Fusion");
#endif

    // Force single instance at once
    QString lockFilePath = QDir::tempPath() + "/Joveler_BatteryLine_f527817e-41bd-43a8-86ca-7a20575297ec.lock";
    qDebug() << lockFilePath;
    QLockFile lockFile(lockFilePath);
    if (!lockFile.tryLock(100))
        SystemHelper::SystemError("Another BatteryLine instance is already running.\nOnly one instance can run at once.");

    BatteryLine line;
    line.show();

    SystemHelper::eventLoopRunning(true);
    return app.exec();
}
