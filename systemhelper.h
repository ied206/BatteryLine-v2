#ifndef BASICIO_H
#define BASICIO_H

#include <QString>
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
    static void SystemError(QString errorMsg);
};

#endif // BASICIO_H
