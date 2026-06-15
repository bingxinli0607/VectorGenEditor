#include "Application.h"
#include "controller/DocumentController.h"
#include "view/MainWindow.h"

#include <QFont>

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setApplicationName("VectorGenEditor");
    setApplicationVersion("1.0.0");
    setOrganizationName("VectorGenEditor");
    setStyle("Fusion");

    QFont appFont(QStringLiteral("Microsoft YaHei UI"));
    if (!appFont.exactMatch()) {
        appFont = QFont(QStringLiteral("Microsoft YaHei"));
        if (!appFont.exactMatch())
            appFont = QFont(QStringLiteral("Segoe UI"));
    }
    appFont.setPointSizeF(11.0);
    appFont.setStyleHint(QFont::SansSerif);
    setFont(appFont);
}

int Application::run(DocumentController *controller)
{
    MainWindow mainWindow(controller);
    mainWindow.show();
    return exec();
}
