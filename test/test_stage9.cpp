#include <QApplication>
#include <iostream>

#include "model/Document.h"
#include "model/Workspace.h"
#include "model/Geometry.h"
#include "controller/DocumentController.h"
#include "controller/SelectionController.h"
#include "view/CanvasScene.h"
#include "view/ShapeGraphicsItem.h"
#include "view/HandleItem.h"
#include "command/MoveShapesCommand.h"

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() std::cout << "PASS" << std::endl
#define FAIL(msg) do { std::cout << "FAIL: " << msg << std::endl; passed = false; } while (0)

struct Fixture {
    std::unique_ptr<Document> doc;
    CanvasScene *scene = nullptr;
    std::unique_ptr<DocumentController> ctrl;

    Fixture() {
        doc = std::make_unique<Document>();
        scene = new CanvasScene();
        ctrl = std::make_unique<DocumentController>(doc.get(), scene);
        scene->setDocumentController(ctrl.get());
        scene->setDocument(doc.get());
    }

    SelectionController *selCtrl() const {
        return scene->selectionController();
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    bool passed = true;

    std::cout << "=== Stage 9: selection / multi-move / workspace ===" << std::endl;

    TEST("selectAll syncs visual selection state");
    {
        Fixture f;
        f.ctrl->createRect(100, 100, 80, 60);
        f.ctrl->createRect(300, 100, 80, 60);
        f.ctrl->createRect(500, 100, 80, 60);
        f.ctrl->createRect(700, 100, 80, 60);
        f.ctrl->createRect(900, 100, 80, 60);

        auto *sel = f.selCtrl();
        sel->selectAll();
        if (sel->selectionCount() != 5) {
            FAIL("expected 5 selected");
        } else {
            int selectedCount = 0;
            for (auto *item : f.scene->shapeItems()) {
                if (sel->isItemSelected(item) && item->isSelected())
                    ++selectedCount;
            }
            if (selectedCount != 5) { FAIL("not all items report selected"); }
            else { PASS(); }
        }
    }

    TEST("multi-select hides resize handles");
    {
        Fixture f;
        f.ctrl->createRect(100, 100, 80, 60);
        f.ctrl->createRect(300, 100, 80, 60);
        auto *sel = f.selCtrl();
        sel->selectAll();
        int handleCount = 0;
        for (auto *gi : f.scene->items()) {
            if (dynamic_cast<HandleItem*>(gi)) ++handleCount;
        }
        if (handleCount != 0) { FAIL("handles visible for multi-select"); }
        else { PASS(); }
    }

    TEST("MoveShapesCommand undo restores all positions");
    {
        Fixture f;
        const QString id1 = f.ctrl->createRect(100, 100, 80, 60);
        const QString id2 = f.ctrl->createRect(300, 100, 80, 60);
        QVector<ShapeMoveEntry> moves;
        moves.append({id1, QPointF(100, 100), QPointF(150, 140)});
        moves.append({id2, QPointF(300, 100), QPointF(350, 140)});
        f.ctrl->undoStack()->push(new MoveShapesCommand(f.doc.get(), moves));

        auto *s1 = f.doc->findShape(id1);
        auto *s2 = f.doc->findShape(id2);
        if (!s1 || !s2) {
            FAIL("shapes missing");
        } else if (s1->transform().x != 150 || s2->transform().x != 350) {
            FAIL("redo did not apply");
        } else {
            f.ctrl->undoStack()->undo();
            if (s1->transform().x != 100 || s2->transform().x != 300) {
                FAIL("undo did not restore");
            } else {
                PASS();
            }
        }
    }

    TEST("createRect outside workspace expands workspace");
    {
        Fixture f;
        const QRectF ws = f.doc->workspaceRect();
        const QString id = f.ctrl->createRect(ws.right() + 50, ws.bottom() + 50, 80, 60);
        if (id.isEmpty()) { FAIL("shape not created outside workspace"); }
        else if (!f.doc->findShape(id)) { FAIL("shape missing after create"); }
        else if (!f.doc->workspaceRect().contains(Geometry::sceneBounds(*f.doc->findShape(id)))) {
            FAIL("workspace did not expand to fit shape");
        } else {
            PASS();
        }
    }

    TEST("clampTopLeft keeps shape inside workspace");
    {
        Fixture f;
        const QString id = f.ctrl->createRect(100, 100, 80, 60);
        auto *shape = f.doc->findShape(id);
        if (!shape) {
            FAIL("shape missing");
        } else {
            const QRectF ws = f.doc->workspaceRect();
            const QPointF clamped = Workspace::clampTopLeft(
                *shape, QPointF(ws.right() + 100, ws.bottom() + 100), ws);
            shape->setTransform({clamped.x(), clamped.y(), 80, 60, 0});
            const QRectF bounds = Geometry::sceneBounds(*shape);
            if (!ws.contains(bounds)) { FAIL("clamped shape still outside"); }
            else { PASS(); }
        }
    }

    std::cout << (passed ? "\nALL TESTS PASSED\n" : "\nSOME TESTS FAILED\n");
    return passed ? 0 : 1;
}
