#include "systemhelper.h"

#include <QMessageBox>
#include <QCoreApplication>
#include <QDebug>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

bool SystemHelper::m_eventLoopRunning = false;

SystemHelper::SystemHelper()
{
    m_eventLoopRunning = false;
}

int SystemHelper::WhatBitOS()
{
    return sizeof(void*) * 8;
}

// Linux-Style Hex Dump
void SystemHelper::BinaryDump(const uint8_t buf[], const uint32_t bufsize)
{
    uint32_t base = 0;
    uint32_t interval = 16;
    while (base < bufsize)
    {
        if (base + 16 < bufsize)
            interval = 16;
        else
            interval = bufsize - base;

        printf("0x%04x:   ", base);
        for (uint32_t i = base; i < base + 16; i++) // i for dump
        {
            if (i < base + interval)
                printf("%02x", buf[i]);
            else
            {
                putchar(' ');
                putchar(' ');
            }

            if ((i+1) % 2 == 0)
                putchar(' ');
            if ((i+1) % 8 == 0)
                putchar(' ');
        }
        putchar(' ');
        putchar(' ');
        for (uint32_t i = base; i < base + 16; i++) // i for dump
        {
            if (i < base + interval)
            {
                if (0x20 <= buf[i] && buf[i] <= 0x7E)
                    printf("%c", buf[i]);
                else
                    putchar('.');
            }
            else
            {
                putchar(' ');
                putchar(' ');
            }

            if ((i+1) % 8 == 0)
                putchar(' ');
        }
        putchar('\n');


        if (base + 16 < bufsize)
            base += 16;
        else
            base = bufsize;
    }

    return;
}

// Get compiled year from gcc's __DATE__
int SystemHelper::CompileYear()
{
    const char macro[16] = __DATE__;
    char stmp[8] = {0};

    stmp[0] = macro[7];
    stmp[1] = macro[8];
    stmp[2] = macro[9];
    stmp[3] = macro[10];
    stmp[4] = '\0';

    return atoi(stmp);
}

// Get compiled month from gcc's __DATE__
int SystemHelper::CompileMonth()
{
    const char macro[16] = __DATE__;
    const char smonth[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int i = 0;

    for (i = 0; i < 12; i++)
    {
        if (strstr(macro, smonth[i]) != NULL)
            return i + 1;
    }

    // return -1 for error
    return -1;
}

// Get compiled day from gcc's __DATE__
int SystemHelper::CompileDay()
{
    const char macro[16] = __DATE__;
    char stmp[4] = {0};

    stmp[0] = macro[4];
    stmp[1] = macro[5];
    stmp[2] = '\0';
    return atoi(stmp);
}

void SystemHelper::SystemError(const QString errorMsg)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("BatteryLine Error");
    msgBox.setText(errorMsg);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    if (SystemHelper::m_eventLoopRunning)
        QCoreApplication::exit(1);
    else
        exit(1);
}

void SystemHelper::eventLoopRunning(bool value)
{
    SystemHelper::m_eventLoopRunning = value;
}

bool SystemHelper::setEventLoopRunning()
{
    return SystemHelper::m_eventLoopRunning;
}

QString SystemHelper::RGB_QColorToQString(const QColor color)
{
    return QString("%1, %2, %3")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue());
}

QColor SystemHelper::RGB_QStringToQColor(const QString str)
{
    QColor color;

    QStringList list = str.trimmed().split(",");
    if (list.count() != 3)
    {
        SystemError("[Setting] Invalid color value, must be form of {R, G, B}");
    }

    for (int i = 0; i < list.count(); i++)
    {
        int val = list[i].trimmed().toInt();
        switch (i)
        {
        case 0:
            color.setRed(val);
            break;
        case 1:
            color.setGreen(val);
            break;
        case 2:
            color.setBlue(val);
            break;
        }
    }

    return color;
}


