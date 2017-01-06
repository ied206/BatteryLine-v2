#include "var.h"
#include "batteryline.h"
#include "systemhelper.h"
#include "singleinstance.h"
#include <QApplication>
#include <QCommandLineParser>

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

    // Set Program Info
    QCoreApplication::setOrganizationName(BL_ORG_NAME);
    QCoreApplication::setOrganizationDomain(BL_ORG_DOMAIN);
    QCoreApplication::setApplicationName(BL_APP_NAME);
    QCoreApplication::setApplicationVersion(BL_VER_STING);

    // Parse Command Line Option
    QCommandLineParser parser;
    parser.setApplicationDescription("Shows system's battery status as line in screen");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption quietOption({ "q", "quiet"}, "Launch this program without notification.");
    parser.addOption(quietOption);
    parser.process(app);

    // Force single instance at once
    SingleInstance single(BL_LOCKFILE, parser.isSet(quietOption), app);
    (void) single;

    // Line as Window
    BatteryLine line(parser.isSet(quietOption));
    line.show();

    SystemHelper::eventLoopRunning(true);
    return app.exec();
}


