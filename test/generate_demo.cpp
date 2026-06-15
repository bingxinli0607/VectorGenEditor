#include <QApplication>
#include <iostream>
#include "model/Document.h"
#include "model/Layer.h"
#include "model/RectShape.h"
#include "model/EllipseShape.h"
#include "model/PolygonShape.h"
#include "model/PolylineShape.h"
#include "model/TextShape.h"
#include "model/StarShape.h"
#include "model/ArrowShape.h"
#include "serialization/JsonSerializer.h"
#include "serialization/SvgExporter.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Document doc;

    // Layer 1: Basic shapes
    auto &layer1 = doc.layers();
    {
        auto rect = std::make_unique<RectShape>();
        rect->setTransform({50, 50, 150, 100, 0.0});
        rect->setStyle({"#4A90D9", "#333333", 2.0, true});
        layer1.front()->addShape(std::move(rect));

        auto ellipse = std::make_unique<EllipseShape>();
        ellipse->setTransform({250, 50, 180, 120, 0.0});
        ellipse->setStyle({"#E8734A", "#333333", 2.0, true});
        layer1.front()->addShape(std::move(ellipse));

        auto tri = PolygonShape::createTriangle(150, 300, 80);
        tri->setStyle({"#50B86C", "#333333", 2.0, true});
        layer1.front()->addShape(std::move(tri));

        auto pent = PolygonShape::createRegularPolygon(5, 400, 300, 60);
        pent->setStyle({"#9B59B6", "#333333", 2.0, true});
        layer1.front()->addShape(std::move(pent));

        QVector<QPointF> pts = {{500, 50}, {580, 100}, {650, 60}, {700, 120}};
        auto pline = std::make_unique<PolylineShape>();
        pline->setPoints(pts);
        pline->setStyle({"none", "#E74C3C", 3.0, true});
        layer1.front()->addShape(std::move(pline));
    }

    // Layer 2: Text & Stars
    doc.addLayer("Annotations");
    doc.setCurrentLayer(1);
    {
        auto txt = std::make_unique<TextShape>();
        txt->setTransform({50, 400, 300, 50, 0.0});
        txt->setText("VectorGenEditor Demo");
        txt->setStyle({"#2C3E50", "none", 0.0, true});
        doc.currentLayer()->addShape(std::move(txt));

        auto star = std::make_unique<StarShape>();
        star->setParameters(5, 0.5);
        star->generateVertices(400, 450, 50);
        star->setTransform({350, 400, 100, 100, 0.0});
        star->setStyle({"#F1C40F", "#333333", 2.0, true});
        doc.currentLayer()->addShape(std::move(star));

        auto arrow = std::make_unique<ArrowShape>();
        arrow->setParameters(15, 20);
        arrow->setStartEnd(QPointF(500, 430), QPointF(650, 430));
        arrow->setStyle({"#3498DB", "#2C3E50", 2.0, true});
        doc.currentLayer()->addShape(std::move(arrow));
    }

    // Save JSON
    QString jsonPath = "../assets/demo/demo_project.json";
    if (JsonSerializer::saveToFile(doc, jsonPath))
        std::cout << "JSON saved: " << jsonPath.toStdString() << std::endl;
    else
        std::cerr << "FAILED to save JSON: " << jsonPath.toStdString() << std::endl;

    // Export SVG
    QString svgPath = "../assets/demo/demo_export.svg";
    if (SvgExporter::exportToFile(doc, svgPath))
        std::cout << "SVG saved: " << svgPath.toStdString() << std::endl;
    else
        std::cerr << "FAILED to save SVG: " << svgPath.toStdString() << std::endl;

    return 0;
}
