#include <QApplication>
#include <QUndoStack>
#include <iostream>

#include "model/Document.h"
#include "model/Layer.h"
#include "model/Shape.h"
#include "controller/DocumentController.h"
#include "view/CanvasScene.h"
#include "command/AddShapeCommand.h"
#include "command/RemoveShapeCommand.h"
#include "command/MoveShapeCommand.h"
#include "command/ResizeShapeCommand.h"
#include "command/ChangeStyleCommand.h"

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() std::cout << "PASS" << std::endl
#define FAIL(msg) std::cout << "FAIL: " << msg << std::endl; passed = false

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
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    bool passed = true;

    std::cout << "\n=== Stage 4 Self-Test (Undo/Redo) ===\n" << std::endl;

    // --- Test 1: AddShapeCommand undo/redo ---
    TEST("AddShapeCommand undo removes shape, redo re-adds");
    {
        Fixture f;
        f.ctrl->createRect(10, 20, 100, 50);
        QString id = f.doc->layers().front()->shapes().front()->id();

        if (f.doc->layers().front()->shapes().size() != 1) { FAIL("shape not created"); }
        else {
            f.ctrl->undoStack()->undo();
            if (f.doc->findShape(id)) { FAIL("shape exists after undo"); }
            else {
                f.ctrl->undoStack()->redo();
                if (!f.doc->findShape(id)) { FAIL("shape not restored after redo"); }
                else { PASS(); }
            }
        }
    }

    // --- Test 2: RemoveShapeCommand undo re-creates shape ---
    TEST("RemoveShapeCommand undo re-creates shape with same ID");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 100, 100);
        QString id = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->deleteShape(id);

        if (f.doc->findShape(id)) { FAIL("shape not deleted"); }
        else {
            f.ctrl->undoStack()->undo();
            Shape *restored = f.doc->findShape(id);
            if (!restored) { FAIL("shape not restored after undo delete"); }
            else if (restored->id() != id) { FAIL("restored shape has different ID"); }
            else { PASS(); }
        }
    }

    // --- Test 3: MoveShapeCommand undo restores old position ---
    TEST("MoveShapeCommand undo restores old position");
    {
        Fixture f;
        f.ctrl->createRect(10, 10, 50, 50);
        QString id = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->updateShapePosition(id, 10.0, 10.0, 200.0, 300.0);

        auto t = f.doc->findShape(id)->transform();
        if (t.x != 200.0 || t.y != 300.0) { FAIL("move not applied"); }
        else {
            f.ctrl->undoStack()->undo();
            t = f.doc->findShape(id)->transform();
            if (t.x != 10.0 || t.y != 10.0) { FAIL("undo did not restore old position"); }
            else { PASS(); }
        }
    }

    // --- Test 4: ResizeShapeCommand undo/redo ---
    TEST("ResizeShapeCommand undo restores old size");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        QString id = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->updateShapeGeometry(id, 0, 0, 50, 50, 0, 10, 20, 300, 400, 45.0);

        auto t = f.doc->findShape(id)->transform();
        if (t.width != 300.0) { FAIL("resize not applied"); }
        else {
            f.ctrl->undoStack()->undo();
            t = f.doc->findShape(id)->transform();
            if (t.x != 0.0 || t.y != 0.0 || t.width != 50.0 || t.height != 50.0)
            { FAIL("undo did not restore old geometry"); }
            else { PASS(); }
        }
    }

    // --- Test 5: ChangeStyleCommand undo/redo ---
    TEST("ChangeStyleCommand undo restores old style");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        QString id = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->updateShapeStyle(id, "#FF0000", "#00FF00", 5.0);

        auto s = f.doc->findShape(id)->style();
        if (s.fillColor != "#FF0000") { FAIL("style not applied"); }
        else {
            f.ctrl->undoStack()->undo();
            s = f.doc->findShape(id)->style();
            if (s.fillColor != "#4A90D9") { FAIL("undo did not restore old fill"); }
            else { PASS(); }
        }
    }

    // --- Test 6: Multiple commands, sequential undo ---
    TEST("Sequential undo of multiple commands");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        f.ctrl->createEllipse(100, 100, 60, 60);

        size_t count = f.doc->layers().front()->shapes().size();
        if (count != 2) { FAIL("expected 2 shapes, got " + std::to_string(count)); }
        else {
            f.ctrl->undoStack()->undo(); // undo ellipse
            count = f.doc->layers().front()->shapes().size();
            if (count != 1) { FAIL("after undo ellipse: expected 1, got " + std::to_string(count)); }
            else {
                f.ctrl->undoStack()->undo(); // undo rect
                count = f.doc->layers().front()->shapes().size();
                if (count != 0) { FAIL("after undo rect: expected 0, got " + std::to_string(count)); }
                else { PASS(); }
            }
        }
    }

    // --- Test 7: Redo after multi-undo ---
    TEST("Redo restores after multi-undo");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        f.ctrl->createEllipse(100, 100, 60, 60);

        f.ctrl->undoStack()->undo();
        f.ctrl->undoStack()->undo();
        if (f.doc->layers().front()->shapes().size() != 0) { FAIL("undo did not clear"); }
        else {
            f.ctrl->undoStack()->redo(); // redo rect
            if (f.doc->layers().front()->shapes().size() != 1) { FAIL("redo rect failed"); }
            else {
                f.ctrl->undoStack()->redo(); // redo ellipse
                if (f.doc->layers().front()->shapes().size() != 2) { FAIL("redo ellipse failed"); }
                else { PASS(); }
            }
        }
    }

    // --- Test 8: MoveShapeCommand merge ---
    TEST("MoveShapeCommand merges consecutive moves");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        QString id = f.doc->layers().front()->shapes().front()->id();

        // Two consecutive moves of the same shape should merge
        f.ctrl->updateShapePosition(id, 0.0, 0.0, 10.0, 10.0);
        int count1 = f.ctrl->undoStack()->count();

        f.ctrl->updateShapePosition(id, 10.0, 10.0, 20.0, 20.0);
        int count2 = f.ctrl->undoStack()->count();

        // After merging, count should stay the same (or be count1 if first move was the second command)
        // The merge should result in only one move command for this shape
        // Single undo should go back to original position
        f.ctrl->undoStack()->undo();
        auto t = f.doc->findShape(id)->transform();
        if (t.x != 0.0 || t.y != 0.0) { FAIL("merged undo did not restore to original"); }
        else { PASS(); }
    }

    // --- Test 9: undoStack is accessible from controller ---
    TEST("DocumentController exposes undoStack");
    {
        Fixture f;
        if (f.ctrl->undoStack() == nullptr) { FAIL("undoStack is null"); }
        else if (f.ctrl->undoStack()->count() != 0) { FAIL("new stack should be empty"); }
        else { PASS(); }
    }

    // --- Test 10: Clear undo stack capability ---
    TEST("Undo stack count tracks commands");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        f.ctrl->createEllipse(100, 100, 60, 60);
        f.ctrl->createTriangle(200, 200, 80);

        if (f.ctrl->undoStack()->count() != 3) { FAIL("expected 3 commands"); }
        else {
            f.ctrl->undoStack()->clear();
            if (f.ctrl->undoStack()->count() != 0) { FAIL("clear did not empty stack"); }
            else { PASS(); }
        }
    }

    // --- Report ---
    std::cout << "\n=== Stage 4 Self-Test Complete ===" << std::endl;
    if (passed) {
        std::cout << "RESULT: ALL TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "RESULT: SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
