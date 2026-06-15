#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

class Document;
class CanvasScene;
class DocumentController;
class MainWindow;

/// Application wrapper — manages global initialization and configuration.
class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);

    int run(DocumentController *controller);
};

#endif // APPLICATION_H
