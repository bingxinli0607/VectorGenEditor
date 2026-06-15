#include "SvgExporter.h"
#include "model/Artboard.h"
#include "model/Geometry.h"
#include "model/Document.h"
#include "model/Layer.h"
#include "model/Shape.h"
#include "model/PolygonShape.h"
#include "model/PolylineShape.h"
#include "model/LineShape.h"
#include "model/TextShape.h"
#include "model/StarShape.h"
#include "model/ArrowShape.h"

#include <QFile>
#include <QTextStream>

namespace {

QString svgColor(const QString &c)
{
    if (c.isEmpty() || c == "none") return "none";
    return c;
}

QString strokeDashAttr(const Style &s)
{
    return s.lineStyle == QStringLiteral("dash") ? QStringLiteral(" stroke-dasharray=\"6,4\"") : QString();
}

void writeShape(QTextStream &out, const Shape &shape, const QString &indent)
{
    Transform t = shape.transform();
    Style s = shape.style();
    QString type = shape.shapeType();

    QString fill = s.fillColor == "none" ? "none" : svgColor(s.fillColor);
    QString stroke = svgColor(s.strokeColor);
    double sw = s.strokeWidth;
    bool hasFill = (fill != "none");
    const QString dash = strokeDashAttr(s);

    if (type == "Rect") {
        out << indent << QString("<rect x=\"%1\" y=\"%2\" width=\"%3\" height=\"%4\"")
                          .arg(t.x).arg(t.y).arg(t.width).arg(t.height);
        if (t.rotation != 0.0)
            out << QString(" transform=\"rotate(%1 %2 %3)\"")
                       .arg(t.rotation).arg(t.x + t.width / 2).arg(t.y + t.height / 2);
        if (hasFill) out << QString(" fill=\"%1\"").arg(fill);
        else out << " fill=\"none\"";
        out << QString(" stroke=\"%1\" stroke-width=\"%2\"").arg(stroke).arg(sw) << dash;
        out << " />\n";
        return;
    }

    if (type == "Ellipse") {
        double rx = t.width / 2.0;
        double ry = t.height / 2.0;
        double cx = t.x + rx;
        double cy = t.y + ry;
        out << indent << QString("<ellipse cx=\"%1\" cy=\"%2\" rx=\"%3\" ry=\"%4\"")
                          .arg(cx).arg(cy).arg(rx).arg(ry);
        if (t.rotation != 0.0)
            out << QString(" transform=\"rotate(%1 %2 %3)\"")
                       .arg(t.rotation).arg(cx).arg(cy);
        if (hasFill) out << QString(" fill=\"%1\"").arg(fill);
        else out << " fill=\"none\"";
        out << QString(" stroke=\"%1\" stroke-width=\"%2\"").arg(stroke).arg(sw) << dash;
        out << " />\n";
        return;
    }

    if (type == "Polygon" || type == "Star" || type == "Arrow") {
        QString points;
        auto verts = Geometry::toScenePoints(shape.vertices(), t);
        for (int i = 0; i < verts.size(); ++i) {
            if (i > 0) points += " ";
            points += QString::number(verts[i].x(), 'f', 2) + ","
                    + QString::number(verts[i].y(), 'f', 2);
        }
        out << indent << QString("<polygon points=\"%1\"").arg(points);
        if (hasFill) out << QString(" fill=\"%1\"").arg(fill);
        else out << " fill=\"none\"";
        out << QString(" stroke=\"%1\" stroke-width=\"%2\"").arg(stroke).arg(sw) << dash;
        out << " />\n";
        return;
    }

    if (type == "Polyline" || type == "Line") {
        bool closed = false;
        if (type == "Polyline") {
            auto *pl = dynamic_cast<const PolylineShape*>(&shape);
            closed = pl ? pl->closed() : false;
        }
        QString points;
        auto verts = Geometry::toScenePoints(shape.vertices(), t);
        for (int i = 0; i < verts.size(); ++i) {
            if (i > 0) points += " ";
            points += QString::number(verts[i].x(), 'f', 2) + ","
                    + QString::number(verts[i].y(), 'f', 2);
        }
        QString tag = closed ? "polygon" : "polyline";
        out << indent << QString("<%1 points=\"%2\"").arg(tag).arg(points);
        out << " fill=\"none\"";
        out << QString(" stroke=\"%1\" stroke-width=\"%2\"").arg(stroke).arg(sw) << dash;
        out << " />\n";
        return;
    }

    if (type == "Text") {
        auto *txt = dynamic_cast<const TextShape*>(&shape);
        QString text = txt ? txt->text().toHtmlEscaped() : "";
        out << indent << QString("<text x=\"%1\" y=\"%2\"")
                          .arg(t.x).arg(t.y + (txt ? txt->fontSize() : 24));
        if (txt)
            out << QString(" font-family=\"%1\" font-size=\"%2\"")
                       .arg(txt->fontFamily()).arg(txt->fontSize());
        out << QString(" fill=\"%1\"").arg(stroke);
        out << ">" << text << "</text>\n";
    }
}

} // namespace

QString SvgExporter::toSvg(const Document &doc)
{
    const QRectF board = doc.workspaceRect().isEmpty() ? Artboard::rect() : doc.workspaceRect();
    QString result;
    QTextStream out(&result);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << QString("<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
                   "viewBox=\"%1 %2 %3 %4\" width=\"%3\" height=\"%4\">\n")
               .arg(board.x()).arg(board.y()).arg(board.width()).arg(board.height());
    out << "  <defs>\n";
    out << QString("    <clipPath id=\"artboardClip\">\n");
    out << QString("      <rect x=\"%1\" y=\"%2\" width=\"%3\" height=\"%4\"/>\n")
               .arg(board.x()).arg(board.y()).arg(board.width()).arg(board.height());
    out << "    </clipPath>\n";
    out << "  </defs>\n";
    out << "  <g clip-path=\"url(#artboardClip)\">\n";

    for (const auto &layer : doc.layers()) {
        if (!layer->visible()) continue;
        out << "    <g id=\"" << layer->name().toHtmlEscaped() << "\">\n";
        for (const auto &shape : layer->shapes()) {
            if (!shape->style().visible) continue;
            if (!Geometry::intersectsRect(Geometry::sceneBounds(*shape), board))
                continue;
            writeShape(out, *shape, "      ");
        }
        out << "    </g>\n";
    }

    out << "  </g>\n";
    out << "</svg>\n";
    return result;
}

bool SvgExporter::exportToFile(const Document &doc, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out << toSvg(doc);
    file.close();
    return true;
}
