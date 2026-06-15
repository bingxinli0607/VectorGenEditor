#include <QApplication>
#include <iostream>
#include <QTemporaryFile>
#include <QUndoStack>

#include "model/Document.h"
#include "model/RectShape.h"
#include "model/Geometry.h"
#include "model/Workspace.h"
#include "serialization/JsonSerializer.h"
#include "command/ResizeShapeCommand.h"
#include "command/RotateShapeCommand.h"

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() std::cout << "PASS" << std::endl
#define FAIL(msg) do { std::cout << "FAIL: " << msg << std::endl; passed = false; } while (0)

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    bool passed = true;

    std::cout << "=== Stage 10: resize / rotate / JSON ===" << std::endl;

    TEST("ResizeShapeCommand undo/redo");
    {
        Document doc;
        auto s = std::make_unique<RectShape>();
        s->setTransform({100, 100, 80, 60, 0});
        const QString id = s->id();
        doc.addShape(std::move(s));

        Transform oldT{100, 100, 80, 60, 0};
        Transform newT{100, 100, 160, 120, 0};
        QUndoStack stack;
        stack.push(new ResizeShapeCommand(&doc, id, oldT, newT));

        auto *shape = doc.findShape(id);
        if (!shape || !qFuzzyCompare(shape->transform().width, 160.0)) {
            FAIL("redo resize");
        } else {
            stack.undo();
            if (!qFuzzyCompare(shape->transform().width, 80.0)) {
                FAIL("undo resize");
            } else if (!stack.canRedo()) {
                FAIL("stack cannot redo");
            } else {
                stack.redo();
                if (qFuzzyCompare(shape->transform().width, 160.0)
                    && qFuzzyCompare(shape->transform().height, 120.0)) {
                    PASS();
                } else {
                    FAIL("redo after undo");
                }
            }
        }
    }

    TEST("RotateShapeCommand undo/redo");
    {
        Document doc;
        auto s = std::make_unique<RectShape>();
        s->setTransform({100, 100, 80, 60, 0});
        const QString id = s->id();
        doc.addShape(std::move(s));

        QUndoStack stack;
        stack.push(new RotateShapeCommand(&doc, id, 0.0, 45.0));

        auto *shape = doc.findShape(id);
        if (!shape || !qFuzzyCompare(shape->transform().rotation, 45.0)) {
            FAIL("redo rotate");
        } else {
            stack.undo();
            if (!qFuzzyCompare(shape->transform().rotation, 0.0)) {
                FAIL("undo rotate");
            } else {
                stack.redo();
                if (qFuzzyCompare(shape->transform().rotation, 45.0)) PASS();
                else FAIL("redo rotate after undo");
            }
        }
    }

    TEST("resize+rotation JSON round-trip");
    {
        Document doc;
        auto s = std::make_unique<RectShape>();
        s->setTransform({200, 150, 120, 90, 30.0});
        const QString id = s->id();
        doc.addShape(std::move(s));

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        if (!tmp.open()) {
            FAIL("temp file");
        } else {
            const QString path = tmp.fileName();
            tmp.close();
            if (!JsonSerializer::saveToFile(doc, path)) {
                FAIL("save");
            } else {
                auto loaded = JsonSerializer::loadFromFile(path);
                Shape *shape = loaded ? loaded->findShape(id) : nullptr;
                if (!shape) {
                    FAIL("load");
                } else {
                    Transform t = shape->transform();
                    if (t.x == 200 && t.width == 120 && qFuzzyCompare(t.rotation, 30.0)) PASS();
                    else FAIL("transform mismatch after JSON");
                }
            }
        }
    }

    TEST("workspace clamp on resize transform");
    {
        Document doc;
        auto s = std::make_unique<RectShape>();
        s->setTransform({100, 100, 80, 60, 0});
        const QString id = s->id();
        doc.addShape(std::move(s));

        Transform huge{100, 100, 5000, 5000, 0};
        Shape *shape = doc.findShape(id);
        Transform clamped = Workspace::clampTransform(*shape, huge, doc.workspaceRect());
        const QRectF after = Geometry::rotatedBoundingRect(clamped);
        if (doc.workspaceRect().contains(after)) PASS();
        else FAIL("clamped resize outside workspace");
    }

    std::cout << (passed ? "\nALL TESTS PASSED\n" : "\nSOME TESTS FAILED\n");
    return passed ? 0 : 1;
}
