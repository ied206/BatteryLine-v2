#include "singleinstance.h"
#include "systemhelper.h"

#include <QObject>
#include <QDir>
#include <QMessageBox>

SingleInstance::SingleInstance(const QString lockFile, const bool mute, const QApplication& app)
{
    this->lockFile = lockFile;
    lock = new QLockFile(lockFile);
    if (!lock->tryLock(100))
    {
        if (mute)
            SystemHelper::QtExit(1);
        else
            SystemHelper::SystemError("Another BatteryLine instance is already running.\nOnly one instance can run at once.");
    }
    connect(&app, &QCoreApplication::aboutToQuit, this, &SingleInstance::AboutToQuit);
}

SingleInstance::~SingleInstance()
{
    delete lock;
}

void SingleInstance::AboutToQuit()
{
    lock->unlock();
    QFile::remove(lockFile);
}
