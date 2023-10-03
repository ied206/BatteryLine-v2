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

    // sudo apt install libxcb-cursor0
    // qt.qpa.plugin: From 6.5.0, xcb-cursor0 or libxcb-cursor0 is needed to load the Qt xcb platform plugin.
    // qt.qpa.plugin: Could not load the Qt platform plugin "xcb" in "" even though it was found.
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
    parser.setApplicationDescription("\nShows system's battery status as line in screen");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption quietOption({ "q", "quiet"}, "Launch this program without notification.");
    parser.addOption(quietOption);
    parser.process(app);

    // Force single instance at once
    SingleInstance single(BL_LOCKID, app);
    (void) single;

    // Create BatteryLine instance
    BatteryLine line(parser.isSet(quietOption), parser.helpText());
    line.show();

    // Run!
    SystemHelper::eventLoopRunning(true);
    return app.exec();
}
