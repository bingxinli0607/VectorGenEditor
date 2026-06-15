#include "StarShape.h"
#include "Geometry.h"
#include <QJsonArray>
#include <QtMath>

std::unique_ptr<Shape> StarShape::clone() const
{
    auto s = std::make_unique<StarShape>();
    s->setId(id());
    s->setTransform(transform());
    s->setStyle(style());
    s->m_points     = m_points;
    s->m_innerRatio = m_innerRatio;
    s->m_vertices   = m_vertices;
    return s;
}

void StarShape::setParameters(int points, double innerRatio)
{
    m_points     = qMax(points, 3);
    m_innerRatio = qBound(0.1, innerRatio, 0.9);
}

void StarShape::setLocalVertices(const QVector<QPointF> &local)
{
    m_vertices = local;
    if (local.isEmpty()) return;
    const QRectF b = Geometry::boundsFromPoints(local);
    Transform t = transform();
    t.width  = qMax(b.width(), 1.0);
    t.height = qMax(b.height(), 1.0);
    setTransform(t);
}

void StarShape::setVerticesFromScene(const QVector<QPointF> &scene)
{
    if (scene.isEmpty()) return;
    const QRectF b = Geometry::boundsFromPoints(scene);
    Transform t = transform();
    t.x = b.x();
    t.y = b.y();
    t.width  = qMax(b.width(), 1.0);
    t.height = qMax(b.height(), 1.0);
    setTransform(t);
    m_vertices = Geometry::toLocalPoints(scene, QPointF(t.x, t.y));
}

void StarShape::translateBy(double dx, double dy)
{
    Shape::translateBy(dx, dy);
}

void StarShape::scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds)
{
    m_vertices = Geometry::scaleLocalPoints(m_vertices, oldBounds, newBounds);
}

void StarShape::replaceLocalVertices(const QVector<QPointF> &local)
{
    setLocalVertices(local);
}

QVector<QPointF> StarShape::generateVertices(double cx, double cy, double outerR) const
{
    QVector<QPointF> verts;
    double innerR = outerR * m_innerRatio;
    for (int i = 0; i < m_points * 2; ++i) {
        double angle = -M_PI_2 + i * M_PI / m_points;
        double r = (i % 2 == 0) ? outerR : innerR;
        verts.append(QPointF(cx + r * cos(angle), cy + r * sin(angle)));
    }
    return verts;
}

QJsonObject StarShape::typeSpecificToJson() const
{
    QJsonObject obj;
    obj["coordSpace"] = "local";
    obj["points"]     = m_points;
    obj["innerRatio"] = m_innerRatio;
    QJsonArray arr;
    for (const auto &p : m_vertices) {
        QJsonObject pt;
        pt["x"] = p.x(); pt["y"] = p.y();
        arr.append(pt);
    }
    obj["vertices"] = arr;
    return obj;
}

void StarShape::typeSpecificFromJson(const QJsonObject &obj)
{
    m_points     = obj["points"].toInt(5);
    m_innerRatio = obj["innerRatio"].toDouble(0.5);
    QVector<QPointF> verts;
    QJsonArray arr = obj["vertices"].toArray();
    for (const auto &v : arr) {
        QJsonObject pt = v.toObject();
        verts.append(QPointF(pt["x"].toDouble(), pt["y"].toDouble()));
    }
    if (verts.isEmpty()) return;

    const bool isLocal = obj["coordSpace"].toString() == "local"
                         || !Geometry::looksLikeAbsoluteCoords(verts, transform());
    if (isLocal) {
        setLocalVertices(verts);
    } else {
        setVerticesFromScene(verts);
    }
}
