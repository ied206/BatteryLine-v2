#ifndef BASICIO_H
#define BASICIO_H

#include <QString>
#include <QColor>
#include <cstdint>

class SystemHelper
{
public:
    SystemHelper();
    static int WhatBitOS();
    static void BinaryDump(const uint8_t buf[], const uint32_t bufsize);
    static int CompileYear();
    static int CompileMonth();
    static int CompileDay();
    static void SystemError(const QString errorMsg);
    static QString RGB_QColorToQString(const QColor color);
    static QColor RGB_QStringToQColor(const QString str);

    static void eventLoopRunning(bool value);
    static bool setEventLoopRunning();

private:
    static bool m_eventLoopRunning;
};

#endif // BASICIO_H
