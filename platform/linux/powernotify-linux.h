#ifndef POWERNOTIFY_LINUX_H
#define POWERNOTIFY_LINUX_H

#include <QObject>
#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

class PowerNotify : public QObject
{
    Q_OBJECT

public:
    PowerNotify();
    ~PowerNotify();

signals:
    void RedrawSignal();

private slots:
    void BatteryInfoChanged(QString, QVariantMap changedProperties, QStringList);
    void ACLineInfoChanged(QString, QVariantMap changedProperties, QStringList);
};

#endif // POWERNOTIFY_LINUX_H

