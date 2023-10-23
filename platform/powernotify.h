#ifndef POWERNOTIFY_H
#define POWERNOTIFY_H

#include <QObject>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif
#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

class PowerNotify : public QObject
{
    Q_OBJECT

public:
    PowerNotify() { };
    virtual ~PowerNotify() { };
    static PowerNotify* CreateInstance();

    virtual bool Register(void* handle) = 0;
    virtual bool Unregister() = 0;

signals:
    // RedrawSignal is fired only in Linux
    // Subscribe QWidget::nativeEvent() and watch WM_POWERBROADCAST in Windows
    void RedrawSignal();
};

#ifdef Q_OS_WIN
class PowerNotifyWin : public PowerNotify
{
public:
    PowerNotifyWin();
    virtual ~PowerNotifyWin();

    virtual bool Register(void* handle);
    virtual bool Unregister();

protected:
    HWND m_hWnd;
    HANDLE m_notPowerSrc;
    HANDLE m_notBatPer;
};
#endif

#ifdef Q_OS_LINUX
class PowerNotifyLinux : public PowerNotify
{
    Q_OBJECT

public:
    PowerNotifyLinux();
    virtual ~PowerNotifyLinux();

    virtual bool Register(void* handle);
    virtual bool Unregister();

private slots:
    void BatteryInfoChanged(QString, QVariantMap changedProperties, QStringList);
    void ACLineInfoChanged(QString, QVariantMap changedProperties, QStringList);

private:
    QList<QString> m_LinePower;
};
#endif


#endif // POWERNOTIFY_H
