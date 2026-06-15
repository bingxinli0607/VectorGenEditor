#include <QApplication>
#include <iostream>

#include "model/Document.h"
#include "model/Layer.h"
#include "model/Shape.h"
#include "controller/DocumentController.h"
#include "view/CanvasScene.h"

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

    std::cout << "\n=== Stage 5 Self-Test (Copy/Paste & Layers) ===\n" << std::endl;

    // --- Test 1: copyShape + pasteShape ---
    TEST("Copy then paste creates new shape with offset");
    {
        Fixture f;
        f.ctrl->createRect(10, 20, 100, 50);
        QString origId = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->copyShape(origId);

        f.ctrl->pasteShape();

        auto *layer = f.doc->layers().front().get();
        if (layer->shapes().size() != 2) { FAIL("expected 2 shapes"); }
        else {
            auto &shapes = layer->shapes();
            // Pasted shape should have offset position
            auto t2 = shapes[1]->transform();
            if (t2.x != 30.0 || t2.y != 40.0)
            { FAIL("pasted shape not offset"); }
            else if (shapes[0]->id() == shapes[1]->id())
            { FAIL("pasted shape has same ID as original"); }
            else if (shapes[1]->shapeType() != "Rect")
            { FAIL("pasted shape has wrong type"); }
            else { PASS(); }
        }
    }

    // --- Test 2: pasteShape is undoable ---
    TEST("Paste is undoable via QUndoStack");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        f.ctrl->copyShape(f.doc->layers().front()->shapes().front()->id());
        f.ctrl->pasteShape();

        size_t count = f.doc->layers().front()->shapes().size();
        if (count != 2) { FAIL("paste did not work"); }
        else {
            f.ctrl->undoStack()->undo();
            count = f.doc->layers().front()->shapes().size();
            if (count != 1) { FAIL("undo paste failed, got " + std::to_string(count)); }
            else { PASS(); }
        }
    }

    // --- Test 3: copyShape with no selection is safe ---
    TEST("copyShape with invalid ID does not crash");
    {
        Fixture f;
        f.ctrl->copyShape("nonexistent-id");
        f.ctrl->pasteShape(); // should do nothing since clipboard is empty
        PASS();
    }

    // --- Test 4: paste without copy does nothing ---
    TEST("Paste with empty clipboard does nothing");
    {
        Fixture f;
        f.ctrl->pasteShape();
        if (f.doc->layers().front()->shapes().size() != 0)
        { FAIL("paste with empty clipboard created shape"); }
        else { PASS(); }
    }

    // --- Test 5: Add layer ---
    TEST("Document::addLayer creates new layer");
    {
        Fixture f;
        int before = f.doc->layerCount();
        f.doc->addLayer("Test Layer");
        if (f.doc->layerCount() != before + 1) { FAIL("layer not added"); }
        else if (f.doc->layerAt(1)->name() != "Test Layer") { FAIL("wrong layer name"); }
        else { PASS(); }
    }

    // --- Test 6: Remove layer ---
    TEST("Document::removeLayer removes layer");
    {
        Fixture f;
        f.doc->addLayer("Extra");
        int before = f.doc->layerCount();
        f.doc->removeLayer(1);
        if (f.doc->layerCount() != before - 1) { FAIL("layer not removed"); }
        else { PASS(); }
    }

    // --- Test 7: Cannot remove last layer ---
    TEST("Cannot remove last remaining layer");
    {
        Fixture f;
        if (f.doc->layerCount() != 1) { FAIL("expected 1 initial layer"); }
        else {
            f.doc->removeLayer(0);
            if (f.doc->layerCount() != 1) { FAIL("last layer was removed"); }
            else { PASS(); }
        }
    }

    // --- Test 8: Switch current layer ---
    TEST("setCurrentLayer switches active layer");
    {
        Fixture f;
        f.doc->addLayer("Layer 2");
        f.doc->setCurrentLayer(1);
        if (f.doc->currentLayerIndex() != 1) { FAIL("layer not switched"); }
        else if (f.doc->currentLayer()->name() != "Layer 2") { FAIL("wrong current layer"); }
        else { PASS(); }
    }

    // --- Test 9: Shapes added to current layer ---
    TEST("Shapes are added to current layer");
    {
        Fixture f;
        f.doc->addLayer("Layer 2");
        f.doc->setCurrentLayer(1);
        f.ctrl->createRect(0, 0, 50, 50);

        // Layer 1 should be empty, Layer 2 should have the shape
        if (f.doc->layerAt(0)->shapes().size() != 0) { FAIL("layer 1 should be empty"); }
        else if (f.doc->layerAt(1)->shapes().size() != 1) { FAIL("layer 2 should have shape"); }
        else { PASS(); }
    }

    // --- Test 10: Layer rename ---
    TEST("Layer::setName renames layer");
    {
        Fixture f;
        auto *layer = f.doc->currentLayer();
        layer->setName("Renamed");
        if (layer->name() != "Renamed") { FAIL("rename failed"); }
        else { PASS(); }
    }

    // --- Test 11: Copy from one layer, paste to another ---
    TEST("Copy from layer 1, switch layer, paste to layer 2");
    {
        Fixture f;
        f.ctrl->createRect(10, 20, 100, 50);
        QString id = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->copyShape(id);

        f.doc->addLayer("Target Layer");
        f.doc->setCurrentLayer(1);
        f.ctrl->pasteShape();

        if (f.doc->layerAt(0)->shapes().size() != 1) { FAIL("original layer lost shape"); }
        else if (f.doc->layerAt(1)->shapes().size() != 1) { FAIL("paste target has no shape"); }
        else { PASS(); }
    }

    // --- Report ---
    std::cout << "\n=== Stage 5 Self-Test Complete ===" << std::endl;
    if (passed) {
        std::cout << "RESULT: ALL TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "RESULT: SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
