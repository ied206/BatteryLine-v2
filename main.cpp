#include "batteryline.h"
#include <QApplication>

#ifdef Q_OS_LINUX
#include <QStyle>
#include <QStyleFactory>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef Q_OS_LINUX
    // Qt 5.7 does not have gtk theme anymore, so in that case, fallback to Fusion theme
    QStringList list = QStyleFactory::keys();
    if (list.contains("gtk", Qt::CaseInsensitive))
        QApplication::setStyle("gtk");
    else if (list.contains("Fusion", Qt::CaseInsensitive))
        QApplication::setStyle("Fusion");
#endif

    BatteryLine line;
    line.show();

    return app.exec();
}
