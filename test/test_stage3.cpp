#include <QApplication>
#include <iostream>

#include "model/Document.h"
#include "model/Layer.h"
#include "model/Shape.h"
#include "controller/DocumentController.h"
#include "controller/SelectionController.h"
#include "view/CanvasScene.h"
#include "view/ShapeGraphicsItem.h"
#include "view/HandleItem.h"
#include "view/PropertyPanel.h"

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() std::cout << "PASS" << std::endl
#define FAIL(msg) std::cout << "FAIL: " << msg << std::endl; passed = false

static int countHandleItems(CanvasScene *scene, ShapeGraphicsItem *parent)
{
    int count = 0;
    for (auto *item : scene->items()) {
        auto *h = dynamic_cast<HandleItem*>(item);
        if (h && h->targetShape() == parent) count++;
    }
    return count;
}

// Setup helper: creates Document, CanvasScene, DocumentController in correct order
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
        return qobject_cast<SelectionController*>(scene->selectionController());
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    bool passed = true;

    std::cout << "\n=== Stage 3 Self-Test ===\n" << std::endl;

    // --- Test 1: DocumentController createRect stores correct geometry ---
    TEST("DocumentController::createRect stores correct geometry");
    {
        Fixture f;
        f.ctrl->createRect(10, 20, 100, 50);
        auto *layer = f.doc->layers().front().get();
        auto *s = layer->shapes().front().get();
        auto t = s->transform();
        if (s->shapeType() != "Rect") { FAIL("wrong type"); }
        else if (t.x != 10 || t.y != 20 || t.width != 100 || t.height != 50)
        { FAIL("wrong geometry"); }
        else { PASS(); }
    }

    // --- Test 2: CanvasScene creates ShapeGraphicsItem on shapeAdded ---
    TEST("CanvasScene creates ShapeGraphicsItem on shapeAdded");
    {
        Fixture f;
        f.ctrl->createEllipse(50, 60, 120, 80);
        auto id = f.doc->layers().front()->shapes().front()->id();
        auto *item = f.scene->itemForShape(id);
        if (!item) { FAIL("no ShapeGraphicsItem in scene"); }
        else if (item->modelShape()->shapeType() != "Ellipse") { FAIL("wrong item type"); }
        else { PASS(); }
    }

    // --- Test 3: ShapeGraphicsItem::modelShape() returns correct shape ---
    TEST("ShapeGraphicsItem::modelShape() returns correct shape");
    {
        Fixture f;
        f.ctrl->createTriangle(100, 100, 80);
        auto id = f.doc->layers().front()->shapes().front()->id();
        auto *item = f.scene->itemForShape(id);
        if (!item->modelShape()) { FAIL("modelShape() returns null"); }
        else if (item->modelShape()->shapeType() != "Polygon") { FAIL("wrong shape type"); }
        else { PASS(); }
    }

    // --- Test 4: SelectionController select/deselectAll ---
    TEST("SelectionController select/deselectAll");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        f.ctrl->createEllipse(100, 0, 50, 50);
        auto id1 = f.doc->layers().front()->shapes()[0]->id();
        auto *item = f.scene->itemForShape(id1);
        auto *selCtrl = f.selCtrl();

        if (!selCtrl) { FAIL("no SelectionController"); }
        else {
            selCtrl->select(item);
            if (!selCtrl->hasSelection()) { FAIL("hasSelection() false"); }
            else if (selCtrl->primarySelected() != item) { FAIL("wrong primary"); }
            else {
                selCtrl->deselectAll();
                if (selCtrl->hasSelection()) { FAIL("still selected after deselectAll"); }
                else { PASS(); }
            }
        }
    }

    // --- Test 5: HandleItems created on select, removed on deselect ---
    TEST("HandleItems created on select, removed on deselect");
    {
        Fixture f;
        f.ctrl->createRect(10, 10, 100, 100);
        auto id = f.doc->layers().front()->shapes().front()->id();
        auto *item = f.scene->itemForShape(id);
        auto *selCtrl = f.selCtrl();

        if (!selCtrl) { FAIL("no SelectionController"); }
        else {
            selCtrl->select(item);
            int handleCount = countHandleItems(f.scene, item);
            if (handleCount < 8) { FAIL("expected >= 8 handles, got " + std::to_string(handleCount)); }
            else {
                selCtrl->deselectAll();
                handleCount = countHandleItems(f.scene, item);
                if (handleCount != 0) { FAIL("handles not removed on deselect"); }
                else { PASS(); }
            }
        }
    }

    // --- Test 6: PropertyPanel setShape/clearShape no-crash ---
    TEST("PropertyPanel setShape/clearShape does not crash");
    {
        Fixture f;
        f.ctrl->createRect(30, 40, 200, 100);
        auto *shape = f.doc->layers().front()->shapes().front().get();

        PropertyPanel panel(f.ctrl.get());
        panel.setShape(shape);
        panel.clearShape();
        PASS();
    }

    // --- Test 7: DocumentController::updateShapePosition ---
    TEST("DocumentController::updateShapePosition updates transform");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        auto id = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->updateShapePosition(id, 0.0, 0.0, 100.0, 200.0);
        auto t = f.doc->findShape(id)->transform();
        if (t.x != 100.0 || t.y != 200.0) { FAIL("position not updated"); }
        else { PASS(); }
    }

    // --- Test 8: DocumentController::updateShapeGeometry ---
    TEST("DocumentController::updateShapeGeometry updates all transform fields");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        auto id = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->updateShapeGeometry(id, 0, 0, 50, 50, 0, 10, 20, 300, 400, 45.0);
        auto t = f.doc->findShape(id)->transform();
        if (t.x != 10 || t.y != 20 || t.width != 300 || t.height != 400 || t.rotation != 45.0)
        { FAIL("geometry not fully updated"); }
        else { PASS(); }
    }

    // --- Test 9: DocumentController::updateShapeStyle ---
    TEST("DocumentController::updateShapeStyle updates fill/stroke/width");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        auto id = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->updateShapeStyle(id, "#FF0000", "#0000FF", 3.0);
        auto s = f.doc->findShape(id)->style();
        if (s.fillColor != "#FF0000") { FAIL("fill not updated"); }
        else if (s.strokeColor != "#0000FF") { FAIL("stroke not updated"); }
        else if (s.strokeWidth != 3.0) { FAIL("stroke width not updated"); }
        else { PASS(); }
    }

    // --- Test 10: deleteShape removes from Document and CanvasScene ---
    TEST("deleteShape removes from Document and CanvasScene");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        auto id = f.doc->layers().front()->shapes().front()->id();

        if (!f.scene->itemForShape(id)) { FAIL("item not in scene before delete"); }
        else {
            f.ctrl->deleteShape(id);
            if (f.doc->findShape(id)) { FAIL("shape still in document"); }
            else if (f.scene->itemForShape(id)) { FAIL("item still in scene"); }
            else { PASS(); }
        }
    }

    // --- Test 11: Multiple shapes sync ---
    TEST("Multiple shapes synced in scene");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        f.ctrl->createEllipse(100, 0, 60, 60);
        f.ctrl->createTriangle(200, 200, 80);

        auto *layer = f.doc->layers().front().get();
        if (layer->shapes().size() != 3) { FAIL("wrong doc count"); }
        else {
            int sceneCount = 0;
            for (const auto &s : layer->shapes()) {
                if (f.scene->itemForShape(s->id())) sceneCount++;
            }
            if (sceneCount != 3) { FAIL("wrong scene count: " + std::to_string(sceneCount)); }
            else { PASS(); }
        }
    }

    // --- Test 12: HandleItem::anchorForRole ---
    TEST("HandleItem::anchorForRole returns correct positions");
    {
        QRectF bounds(0, 0, 100, 80);
        QPointF tl = HandleItem::anchorForRole(HandleRole::ResizeTL, bounds);
        QPointF br = HandleItem::anchorForRole(HandleRole::ResizeBR, bounds);
        QPointF rot = HandleItem::anchorForRole(HandleRole::Rotate, bounds);

        if (tl != QPointF(0, 0)) { FAIL("TL anchor wrong"); }
        else if (br != QPointF(100, 80)) { FAIL("BR anchor wrong"); }
        else if (rot.y() >= bounds.top()) { FAIL("rotate handle should be above bounds"); }
        else { PASS(); }
    }

    // --- Test 13: SelectionController::selectionChanged signal ---
    TEST("SelectionController emits selectionChanged signal");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        auto id = f.doc->layers().front()->shapes().front()->id();
        auto *item = f.scene->itemForShape(id);
        auto *selCtrl = f.selCtrl();

        if (!selCtrl) { FAIL("no SelectionController"); }
        else {
            bool fired = false;
            QObject::connect(selCtrl, &SelectionController::selectionChanged,
                             [&]() { fired = true; });
            selCtrl->select(item);
            if (!fired) { FAIL("not emitted on select"); }
            else {
                fired = false;
                selCtrl->deselectAll();
                if (!fired) { FAIL("not emitted on deselectAll"); }
                else { PASS(); }
            }
        }
    }

    // --- Test 14: Polyline creation ---
    TEST("Polyline creation and scene sync");
    {
        Fixture f;
        QVector<QPointF> pts = {{0,0}, {50,50}, {100,0}};
        f.ctrl->createPolyline(pts);
        auto *layer = f.doc->layers().front().get();
        if (layer->shapes().front()->shapeType() != "Polyline") { FAIL("wrong type"); }
        else if (!f.scene->itemForShape(layer->shapes().front()->id())) { FAIL("no item"); }
        else { PASS(); }
    }

    // --- Test 15: Text shape ---
    TEST("Text shape creation and scene sync");
    {
        Fixture f;
        f.ctrl->createText(100, 200, "Hello World");
        auto *layer = f.doc->layers().front().get();
        if (layer->shapes().front()->shapeType() != "Text") { FAIL("wrong type"); }
        else if (!f.scene->itemForShape(layer->shapes().front()->id())) { FAIL("no item"); }
        else { PASS(); }
    }

    // --- Test 16: Star shape ---
    TEST("Star shape creation");
    {
        Fixture f;
        f.ctrl->createStar(200, 200, 80, 5, 0.5);
        auto *layer = f.doc->layers().front().get();
        if (layer->shapes().front()->shapeType() != "Star") { FAIL("wrong type"); }
        else { PASS(); }
    }

    // --- Test 17: Arrow shape ---
    TEST("Arrow shape creation");
    {
        Fixture f;
        f.ctrl->createArrow(QPointF(0, 0), QPointF(200, 100));
        auto *layer = f.doc->layers().front().get();
        if (layer->shapes().front()->shapeType() != "Arrow") { FAIL("wrong type"); }
        else { PASS(); }
    }

    // --- Test 18: updateShapeVisible ---
    TEST("DocumentController::updateShapeVisible");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        auto id = f.doc->layers().front()->shapes().front()->id();
        f.ctrl->updateShapeVisible(id, false);
        if (f.doc->findShape(id)->style().visible) { FAIL("visible should be false"); }
        else { PASS(); }
    }

    // --- Test 19: shapeChanged signal triggers item update ---
    TEST("shapeChanged triggers ShapeGraphicsItem::updateFromModel");
    {
        Fixture f;
        f.ctrl->createRect(10, 10, 50, 50);
        auto id = f.doc->layers().front()->shapes().front()->id();
        auto *item = f.scene->itemForShape(id);

        f.ctrl->updateShapePosition(id, 10.0, 10.0, 300.0, 400.0);
        auto t = item->modelShape()->transform();
        if (t.x != 300 || t.y != 400) { FAIL("item not synced after model update"); }
        else { PASS(); }
    }

    // --- Test 20: deselectAll handles item removal ---
    TEST("deselectAll removes all handles");
    {
        Fixture f;
        f.ctrl->createRect(0, 0, 50, 50);
        auto *selCtrl = f.selCtrl();
        auto id = f.doc->layers().front()->shapes().front()->id();
        auto *item = f.scene->itemForShape(id);

        selCtrl->select(item);
        int afterSelect = countHandleItems(f.scene, item);
        selCtrl->deselectAll();
        int afterDeselect = countHandleItems(f.scene, item);

        if (afterSelect == 0) { FAIL("no handles after select"); }
        else if (afterDeselect != 0) { FAIL("handles remain after deselectAll"); }
        else { PASS(); }
    }

    // --- Report ---
    std::cout << "\n=== Stage 3 Self-Test Complete ===" << std::endl;
    if (passed) {
        std::cout << "RESULT: ALL TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "RESULT: SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
