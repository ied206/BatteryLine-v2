#ifndef BASICIO_H
#define BASICIO_H

#include <QString>
#include <QColor>
#include <cstdint>

class SystemHelper
{
public:
    SystemHelper();

    static QString OSName();
    static QString OSArch();
    static QString ProcArch();
    static void BinaryDump(const uint8_t buf[], const uint32_t bufsize);
    static int CompileYear();
    static int CompileMonth();
    static int CompileDay();
    static void SystemWarning(const QString& warnMsg);
    static void SystemError(const QString& errorMsg);
    static void QtExit(int code = 0);
    static QString RGB_QColorToQString(const QColor color);
    static QColor RGB_QStringToQColor(const QString str);

    static void eventLoopRunning(bool value);
    static bool setEventLoopRunning();

private:
    static bool m_eventLoopRunning;
};

#endif // BASICIO_H
