#include "PolylineShape.h"
#include "Geometry.h"
#include <QJsonArray>

std::unique_ptr<Shape> PolylineShape::clone() const
{
    auto s = std::make_unique<PolylineShape>();
    s->setId(id());
    s->setTransform(transform());
    s->setStyle(style());
    s->m_points  = m_points;
    s->m_closed  = m_closed;
    return s;
}

void PolylineShape::setLocalPoints(const QVector<QPointF> &local)
{
    m_points = local;
    if (local.isEmpty()) return;
    const QRectF b = Geometry::boundsFromPoints(local);
    Transform t = transform();
    t.width  = qMax(b.width(), 1.0);
    t.height = qMax(b.height(), 1.0);
    setTransform(t);
}

void PolylineShape::setPointsFromScene(const QVector<QPointF> &scene)
{
    if (scene.isEmpty()) return;
    const QRectF b = Geometry::boundsFromPoints(scene);
    Transform t = transform();
    t.x = b.x();
    t.y = b.y();
    t.width  = qMax(b.width(), 1.0);
    t.height = qMax(b.height(), 1.0);
    setTransform(t);
    m_points = Geometry::toLocalPoints(scene, QPointF(t.x, t.y));
}

void PolylineShape::setClosed(bool c) { m_closed = c; }

void PolylineShape::translateBy(double dx, double dy)
{
    Shape::translateBy(dx, dy);
}

void PolylineShape::scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds)
{
    m_points = Geometry::scaleLocalPoints(m_points, oldBounds, newBounds);
}

void PolylineShape::replaceLocalVertices(const QVector<QPointF> &local)
{
    setLocalPoints(local);
}

QJsonObject PolylineShape::typeSpecificToJson() const
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
    obj["closed"] = m_closed;
    return obj;
}

void PolylineShape::typeSpecificFromJson(const QJsonObject &obj)
{
    QVector<QPointF> pts;
    QJsonArray arr = obj["points"].toArray();
    for (const auto &v : arr) {
        QJsonObject pt = v.toObject();
        pts.append(QPointF(pt["x"].toDouble(), pt["y"].toDouble()));
    }
    m_closed = obj["closed"].toBool(false);
    if (pts.isEmpty()) return;

    const bool isLocal = obj["coordSpace"].toString() == "local"
                         || !Geometry::looksLikeAbsoluteCoords(pts, transform());
    if (isLocal) {
        setLocalPoints(pts);
    } else {
        setPointsFromScene(pts);
    }
}
