#include "singleinstance.h"
#include "systemhelper.h"

#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <signal.h>
#endif

SingleInstance::SingleInstance(const QString lockId, const QApplication& app)
{
#ifdef Q_OS_WIN
    // Kill BatteryLine v1.x
    HWND hWndLegacy = FindWindowW(L"Joveler_BatteryLine", 0);
    if (hWndLegacy != NULL) // Running BatteryLine found? Terminate it.
        SendMessageW(hWndLegacy, WM_CLOSE, 0, 0);
#endif

    this->lockId = lockId;
    this->lock = new QLockFile(lockId + ".lock");
    if (lock->tryLock(100))
    { // Lock success, leave process info (Win - hWnd, Linux - pid)
        QFile pidFile(lockId + ".pid");
        if (pidFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&pidFile);
            out.setEncoding(QStringConverter::Utf8);
            out << QCoreApplication::applicationPid();
            pidFile.close();
        }
        else // failure
        { // Silently ignore this, it is not likely to happen.
            // SystemHelper::SystemWarning("BatteryLine is runnable, but it cannot write process id.");
        }
    }
    else
    { // Lock failure, another instance is running - so kill that instance and terminate
        qint64 runningPid = 0;

        QFile pidFile(lockId + ".pid");
        if (pidFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&pidFile);
            in.setEncoding(QStringConverter::Utf8);
            runningPid = in.readLine().trimmed().toInt();
            pidFile.close();
        }
        else // failure
        {
            SystemHelper::SystemError("Another instance of BatteryLine is running.");
        }

        // Try to kill running instance and terminate
#ifdef Q_OS_WIN
        DWORD winPid = static_cast<DWORD>(runningPid);
        HWND hWndIter = FindWindowW(NULL, NULL); // Will iterate all 'hWnd'es to find BatteryLine
        while (hWndIter != NULL)
        {
            DWORD getPid = 0;
            GetWindowThreadProcessId(hWndIter, &getPid);
            if (winPid == getPid && GetParent(hWndIter) == NULL) // Only check top-level window
            {
                SendMessageW(hWndIter, WM_CLOSE, 0, 0);
                break;
            }
            hWndIter = GetWindow(hWndIter, GW_HWNDNEXT);
        }
#elif defined(Q_OS_LINUX)
        pid_t linuxPid = static_cast<pid_t>(runningPid);
        kill(linuxPid, SIGTERM);
#endif

        SystemHelper::QtExit(0);
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
    QFile::remove(lockId + ".lock");
    QFile::remove(lockId + ".pid");
}
