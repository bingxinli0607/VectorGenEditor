#include <QApplication>
#include <iostream>
#include <cmath>

#include "model/Document.h"
#include "model/Layer.h"
#include "model/Geometry.h"
#include "model/PolygonShape.h"
#include "model/StarShape.h"
#include "controller/DocumentController.h"
#include "view/CanvasScene.h"
#include "view/ShapeGraphicsItem.h"
#include "view/HandleItem.h"

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() std::cout << "PASS" << std::endl
#define FAIL(msg) std::cout << "FAIL: " << msg << std::endl; passed = false

static bool near(double a, double b, double eps = 0.5)
{
    return std::abs(a - b) <= eps;
}

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

    std::cout << "\n=== Stage 8 Self-Test (Coordinates & Handles) ===\n" << std::endl;

    TEST("Geometry::localBounds matches transform size");
    {
        Transform t = {10, 20, 120, 80, 0};
        QRectF b = Geometry::localBounds(t);
        if (!near(b.width(), 120) || !near(b.height(), 80)) { FAIL("wrong local bounds"); }
        else { PASS(); }
    }

    TEST("Handle anchors align with localBounds corners");
    {
        QRectF bounds(0, 0, 100, 50);
        if (!near(HandleItem::anchorForRole(HandleRole::ResizeTL, bounds).x(), 0)) { FAIL("TL x"); }
        else if (!near(HandleItem::anchorForRole(HandleRole::ResizeBR, bounds).x(), 100)) { FAIL("BR x"); }
        else if (!near(HandleItem::anchorForRole(HandleRole::ResizeBR, bounds).y(), 50)) { FAIL("BR y"); }
        else if (!near(HandleItem::anchorForRole(HandleRole::ResizeT, bounds).x(), 50)) { FAIL("T center"); }
        else { PASS(); }
    }

    TEST("ShapeGraphicsItem::localBounds matches model transform");
    {
        Fixture f;
        f.ctrl->createEllipse(30, 40, 140, 100);
        auto *shape = f.doc->layers().front()->shapes().front().get();
        auto *item = f.scene->itemForShape(shape->id());
        QRectF lb = item->localBounds();
        auto t = shape->transform();
        if (!near(lb.width(), t.width) || !near(lb.height(), t.height)) { FAIL("item bounds mismatch"); }
        else { PASS(); }
    }

    TEST("Polygon uses local vertices after creation");
    {
        auto poly = PolygonShape::createTriangle(200, 200, 100);
        auto verts = poly->vertices();
        auto t = poly->transform();
        if (verts.isEmpty()) { FAIL("no vertices"); }
        else if (Geometry::looksLikeAbsoluteCoords(verts, t)) { FAIL("still absolute coords"); }
        else { PASS(); }
    }

    TEST("Polygon translateBy moves scene position only");
    {
        auto poly = PolygonShape::createTriangle(200, 200, 100);
        auto t0 = poly->transform();
        auto scene0 = Geometry::toScenePoints(poly->vertices(), t0);
        poly->translateBy(30, 30);
        auto t1 = poly->transform();
        auto scene1 = Geometry::toScenePoints(poly->vertices(), t1);
        if (!near(t1.x, t0.x + 30) || !near(t1.y, t0.y + 30)) { FAIL("transform not moved"); }
        else if (!near(scene1[0].x(), scene0[0].x() + 30)) { FAIL("scene vertex not moved"); }
        else { PASS(); }
    }

    TEST("Star stores non-empty local vertices");
    {
        Fixture f;
        f.ctrl->createStar(300, 300, 60, 5, 0.5);
        auto *star = f.doc->findShape(f.doc->layers().front()->shapes().front()->id());
        if (!star || star->vertices().isEmpty()) { FAIL("star vertices empty"); }
        else { PASS(); }
    }

    TEST("ResizeShapeCommand scales polygon vertices");
    {
        Fixture f;
        f.ctrl->createTriangle(200, 200, 100);
        QString id = f.doc->layers().front()->shapes().front()->id();
        auto *shape = f.doc->findShape(id);
        Transform oldT = shape->transform();
        Transform newT = oldT;
        newT.width *= 2;
        newT.height *= 2;
        f.ctrl->updateShapeGeometry(id,
            oldT.x, oldT.y, oldT.width, oldT.height, oldT.rotation,
            newT.x, newT.y, newT.width, newT.height, newT.rotation);
        auto t = shape->transform();
        if (!near(t.width, oldT.width * 2)) { FAIL("width not doubled"); }
        else { PASS(); }
    }

    TEST("rotatedBoundingRect expands for 45 degree rotation");
    {
        Transform t = {0, 0, 100, 50, 45};
        QRectF axis = Geometry::localBounds(t);
        QRectF rotated = Geometry::rotatedBoundingRect(t);
        if (rotated.width() <= axis.width() || rotated.height() <= axis.height())
        { FAIL("rotated bounds not larger"); }
        else { PASS(); }
    }

    TEST("Json round-trip preserves local polygon vertices");
    {
        auto poly = PolygonShape::createTriangle(150, 150, 80);
        QJsonObject obj = poly->toJson();
        auto loaded = std::make_unique<PolygonShape>();
        loaded->fromJson(obj);
        if (loaded->vertices().isEmpty()) { FAIL("vertices lost"); }
        else if (Geometry::looksLikeAbsoluteCoords(loaded->vertices(), loaded->transform()))
        { FAIL("not local after json"); }
        else { PASS(); }
    }

    std::cout << "\n=== Stage 8 Self-Test Complete ===" << std::endl;
    if (passed) {
        std::cout << "RESULT: ALL TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "RESULT: SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
