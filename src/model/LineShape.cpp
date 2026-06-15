#include "LineShape.h"
#include "Geometry.h"
#include <QJsonArray>
#include <QtMath>

std::unique_ptr<Shape> LineShape::clone() const
{
    auto s = std::make_unique<LineShape>();
    s->setId(id());
    s->setTransform(transform());
    s->setStyle(style());
    s->m_points = m_points;
    return s;
}

void LineShape::setLocalPoints(const QVector<QPointF> &local)
{
    m_points = local;
    if (local.size() < 2) return;
    const QRectF b = Geometry::boundsFromPoints(local);
    Transform t = transform();
    t.width  = qMax(b.width(), 1.0);
    t.height = qMax(b.height(), 1.0);
    setTransform(t);
}

void LineShape::setEndpointsFromScene(const QPointF &start, const QPointF &end)
{
    QVector<QPointF> scene = {start, end};
    const QRectF b = Geometry::boundsFromPoints(scene);
    Transform t = transform();
    t.x = b.x();
    t.y = b.y();
    t.width  = qMax(b.width(), 1.0);
    t.height = qMax(b.height(), 1.0);
    setTransform(t);
    m_points = Geometry::toLocalPoints(scene, QPointF(t.x, t.y));
}

void LineShape::translateBy(double dx, double dy)
{
    Shape::translateBy(dx, dy);
}

void LineShape::scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds)
{
    m_points = Geometry::scaleLocalPoints(m_points, oldBounds, newBounds);
}

void LineShape::replaceLocalVertices(const QVector<QPointF> &local)
{
    setLocalPoints(local);
}

QJsonObject LineShape::typeSpecificToJson() const
{
    QJsonObject obj;
    obj["coordSpace"] = "local";
    QJsonArray arr;
    for (const auto &p : m_points) {
        QJsonObject pt;
        pt["x"] = p.x();
        pt["y"] = p.y();
        arr.append(pt);
    }
    obj["points"] = arr;
    return obj;
}

void LineShape::typeSpecificFromJson(const QJsonObject &obj)
{
    QVector<QPointF> pts;
    const QJsonArray arr = obj["points"].toArray();
    for (const auto &v : arr) {
        const QJsonObject pt = v.toObject();
        pts.append(QPointF(pt["x"].toDouble(), pt["y"].toDouble()));
    }
    if (pts.isEmpty()) return;

    const bool isLocal = obj["coordSpace"].toString() == "local"
                         || !Geometry::looksLikeAbsoluteCoords(pts, transform());
    if (isLocal) {
        setLocalPoints(pts);
    } else if (pts.size() >= 2) {
        setEndpointsFromScene(pts[0], pts[1]);
    }
}
