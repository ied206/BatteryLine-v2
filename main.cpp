#include "batteryline.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BatteryLine line;
    line.show();

    return a.exec();
}
