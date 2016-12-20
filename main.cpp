#include "batteryline.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BatteryLine w;



    w.show();

    // a.processEvents();

    return a.exec();
}
