#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QApplication>
#include <QString>
#include <QLockFile>

class SingleInstance : public QObject
{
    Q_OBJECT

public:
    SingleInstance(const QString lockFile, const bool mute, const QApplication &app);
    ~SingleInstance();
public slots:
    void AboutToQuit();
private:
    QLockFile* lock;
    QString lockFile;
};

#endif // SINGLEINSTANCE_H
