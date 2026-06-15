#include <QApplication>
#include <QDir>
#include <QFile>
#include <iostream>

#include "model/Document.h"
#include "model/Layer.h"
#include "model/Shape.h"
#include "model/RectShape.h"
#include "model/EllipseShape.h"
#include "model/PolygonShape.h"
#include "model/TextShape.h"
#include "serialization/JsonSerializer.h"
#include "serialization/SvgExporter.h"

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() std::cout << "PASS" << std::endl
#define FAIL(msg) std::cout << "FAIL: " << msg << std::endl; passed = false

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    bool passed = true;

    std::cout << "\n=== Stage 7 Self-Test (JSON & SVG) ===\n" << std::endl;

    // --- Test 1: Document::toJson outputs type/version ---
    TEST("Document::toJson includes formatVersion and appName");
    {
        Document doc;
        QJsonObject obj = doc.toJson();
        if (obj["formatVersion"].toInt() != 1) { FAIL("wrong formatVersion"); }
        else if (obj["appName"].toString() != "VectorGenEditor") { FAIL("wrong appName"); }
        else { PASS(); }
    }

    // --- Test 2: RectShape JSON round-trip ---
    TEST("RectShape toJson/fromJson preserves geometry and style");
    {
        Document doc;
        auto &layers = doc.layers();
        auto s = std::make_unique<RectShape>();
        s->setTransform({10, 20, 100, 50, 0.0});
        s->setStyle({"#FF0000", "#0000FF", 3.0, true});
        QString id = s->id();
        layers.front()->addShape(std::move(s));

        QJsonObject docObj = doc.toJson();
        auto loaded = Document::fromJson(docObj);
        auto *shape = loaded->findShape(id);
        if (!shape) { FAIL("shape not found after round-trip"); }
        else {
            auto t = shape->transform();
            auto st = shape->style();
            if (t.x != 10 || t.y != 20) { FAIL("position lost"); }
            else if (t.width != 100 || t.height != 50) { FAIL("size lost"); }
            else if (st.fillColor != "#FF0000") { FAIL("fill lost"); }
            else if (st.strokeColor != "#0000FF") { FAIL("stroke lost"); }
            else if (st.strokeWidth != 3.0) { FAIL("stroke width lost"); }
            else if (shape->shapeType() != "Rect") { FAIL("type lost"); }
            else { PASS(); }
        }
    }

    // --- Test 3: Multiple shapes round-trip ---
    TEST("Multiple shapes survive JSON round-trip");
    {
        Document doc;
        auto &layers = doc.layers();
        auto r = std::make_unique<RectShape>();
        r->setTransform({0, 0, 50, 50});
        layers.front()->addShape(std::move(r));

        auto e = std::make_unique<EllipseShape>();
        e->setTransform({100, 0, 60, 60});
        layers.front()->addShape(std::move(e));

        QJsonObject obj = doc.toJson();
        auto loaded = Document::fromJson(obj);
        if (loaded->layerCount() != 1) { FAIL("layer count wrong"); }
        else if (loaded->layers().front()->shapes().size() != 2) { FAIL("shape count wrong"); }
        else { PASS(); }
    }

    // --- Test 4: Layers survive round-trip ---
    TEST("Layers survive JSON round-trip");
    {
        Document doc;
        doc.addLayer("Background");
        doc.addLayer("Foreground");
        doc.setCurrentLayer(1);

        QJsonObject obj = doc.toJson();
        auto loaded = Document::fromJson(obj);
        if (loaded->layerCount() != 3) { FAIL("layer count wrong"); }
        else if (loaded->layers()[1]->name() != "Background") { FAIL("layer name lost"); }
        else if (loaded->currentLayerIndex() != 1) { FAIL("current layer index lost"); }
        else { PASS(); }
    }

    // --- Test 5: SVG export contains XML header ---
    TEST("SVG export contains XML header and SVG namespace");
    {
        Document doc;
        QString svg = SvgExporter::toSvg(doc);
        if (!svg.contains("<?xml")) { FAIL("no XML header"); }
        else if (!svg.contains("svg xmlns")) { FAIL("no SVG namespace"); }
        else if (!svg.contains("</svg>")) { FAIL("no closing svg tag"); }
        else { PASS(); }
    }

    // --- Test 6: SVG rect element ---
    TEST("SVG export renders Rect as <rect> element");
    {
        Document doc;
        auto r = std::make_unique<RectShape>();
        r->setTransform({10, 20, 100, 50});
        doc.layers().front()->addShape(std::move(r));

        QString svg = SvgExporter::toSvg(doc);
        if (!svg.contains("<rect")) { FAIL("no <rect> element"); }
        else if (!svg.contains("width=\"100\"")) { FAIL("missing width"); }
        else { PASS(); }
    }

    // --- Test 7: SVG ellipse element ---
    TEST("SVG export renders Ellipse as <ellipse> element");
    {
        Document doc;
        auto e = std::make_unique<EllipseShape>();
        e->setTransform({50, 50, 120, 80});
        doc.layers().front()->addShape(std::move(e));

        QString svg = SvgExporter::toSvg(doc);
        if (!svg.contains("<ellipse")) { FAIL("no <ellipse> element"); }
        else { PASS(); }
    }

    // --- Test 8: SVG polygon element for PolygonShape ---
    TEST("SVG export renders Polygon as <polygon> element");
    {
        Document doc;
        auto p = PolygonShape::createTriangle(100, 100, 80);
        doc.layers().front()->addShape(std::move(p));

        QString svg = SvgExporter::toSvg(doc);
        if (!svg.contains("<polygon")) { FAIL("no <polygon> element"); }
        else { PASS(); }
    }

    // --- Test 9: SVG text element ---
    TEST("SVG export renders Text as <text> element");
    {
        Document doc;
        auto t = std::make_unique<TextShape>();
        t->setTransform({10, 20, 200, 40});
        t->setText("Hello");
        doc.layers().front()->addShape(std::move(t));

        QString svg = SvgExporter::toSvg(doc);
        if (!svg.contains("<text")) { FAIL("no <text> element"); }
        else if (!svg.contains("Hello")) { FAIL("text content missing"); }
        else { PASS(); }
    }

    // --- Test 10: SVG respects layer visibility ---
    TEST("SVG export skips invisible layers");
    {
        Document doc;
        doc.addLayer("Hidden");
        doc.layerAt(1)->setVisible(false);
        doc.setCurrentLayer(1);
        auto r = std::make_unique<RectShape>();
        r->setTransform({0, 0, 50, 50});
        doc.currentLayer()->addShape(std::move(r));

        QString svg = SvgExporter::toSvg(doc);
        // Invisible layers are skipped entirely
        if (svg.contains("Hidden")) { FAIL("invisible layer should be excluded"); }
        // But visible layer 1 should still show
        else if (!svg.contains("Layer 1")) { FAIL("visible layer missing"); }
        else { PASS(); }
    }

    // --- Test 11: Save to file ---
    TEST("JsonSerializer::saveToFile writes valid JSON file");
    {
        Document doc;
        auto r = std::make_unique<RectShape>();
        r->setTransform({0, 0, 50, 50});
        doc.layers().front()->addShape(std::move(r));

        QString path = QDir::temp().filePath("test_vge_save.json");
        bool saved = JsonSerializer::saveToFile(doc, path);
        if (!saved) { FAIL("save failed"); }
        else {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) { FAIL("cannot re-open file"); }
            else {
                QByteArray data = file.readAll();
                file.close();
                if (!data.contains("formatVersion")) { FAIL("invalid JSON content"); }
                else { PASS(); }
            }
        }
    }

    // --- Test 12: Load from file ---
    TEST("JsonSerializer::loadFromFile reconstructs Document");
    {
        Document doc;
        auto r = std::make_unique<RectShape>();
        r->setTransform({10, 10, 50, 50});
        QString id = r->id();
        doc.layers().front()->addShape(std::move(r));

        QString path = QDir::temp().filePath("test_vge_load.json");
        JsonSerializer::saveToFile(doc, path);

        auto loaded = JsonSerializer::loadFromFile(path);
        if (!loaded) { FAIL("load returned null"); }
        else if (!loaded->findShape(id)) { FAIL("shape not found"); }
        else { PASS(); }
    }

    // --- Report ---
    std::cout << "\n=== Stage 7 Self-Test Complete ===" << std::endl;
    if (passed) {
        std::cout << "RESULT: ALL TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "RESULT: SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
