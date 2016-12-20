#ifndef POWERNOTIFY_WIN_H
#define POWERNOTIFY_WIN_H

#include <QObject>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

class PowerNotify : public QObject
{
    Q_OBJECT

public:
    PowerNotify(HWND hWnd);
    ~PowerNotify();

public slots:
    void RedrawSignal();

private:
    // fp_Redraw_t RedrawCallback;
    HWND hWnd;
    HANDLE m_notPowerSrc;
    HANDLE m_notBatPer;
};

#endif // POWERNOTIFY_WIN_H

