#include "app/Application.h"
#include "model/Document.h"
#include "view/CanvasScene.h"
#include "controller/DocumentController.h"

int main(int argc, char *argv[])
{
    Application app(argc, argv);

    auto *document = new Document(&app);
    auto *scene = new CanvasScene(&app);
    auto *controller = new DocumentController(document, scene, &app);

    scene->setDocumentController(controller);
    scene->setDocument(document);

    return app.run(controller);
}
