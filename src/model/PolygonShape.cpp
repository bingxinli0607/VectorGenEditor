#include "PolygonShape.h"
#include "Geometry.h"
#include <QJsonArray>
#include <QtMath>

std::unique_ptr<Shape> PolygonShape::clone() const
{
    auto s = std::make_unique<PolygonShape>();
    s->setId(id());
    s->setTransform(transform());
    s->setStyle(style());
    s->m_vertices = m_vertices;
    return s;
}

void PolygonShape::setLocalVertices(const QVector<QPointF> &local)
{
    m_vertices = local;
    if (local.isEmpty()) return;
    const QRectF b = Geometry::boundsFromPoints(local);
    Transform t = transform();
    t.width  = qMax(b.width(), 1.0);
    t.height = qMax(b.height(), 1.0);
    setTransform(t);
}

void PolygonShape::setVerticesFromScene(const QVector<QPointF> &scene)
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

void PolygonShape::translateBy(double dx, double dy)
{
    Shape::translateBy(dx, dy);
}

void PolygonShape::scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds)
{
    m_vertices = Geometry::scaleLocalPoints(m_vertices, oldBounds, newBounds);
}

void PolygonShape::replaceLocalVertices(const QVector<QPointF> &local)
{
    setLocalVertices(local);
}

QJsonObject PolygonShape::typeSpecificToJson() const
{
    QJsonObject obj;
    obj["coordSpace"] = "local";
    QJsonArray arr;
    for (const auto &p : m_vertices) {
        QJsonObject pt;
        pt["x"] = p.x();
        pt["y"] = p.y();
        arr.append(pt);
    }
    obj["vertices"] = arr;
    return obj;
}

void PolygonShape::typeSpecificFromJson(const QJsonObject &obj)
{
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

std::unique_ptr<PolygonShape> PolygonShape::createTriangle(double cx, double cy, double size)
{
    auto s = std::make_unique<PolygonShape>();
    double r = size / 2.0;
    QVector<QPointF> verts;
    for (int i = 0; i < 3; ++i) {
        double angle = -M_PI_2 + i * 2.0 * M_PI / 3.0;
        verts.append(QPointF(cx + r * cos(angle), cy + r * sin(angle)));
    }
    s->setVerticesFromScene(verts);
    return s;
}

std::unique_ptr<PolygonShape> PolygonShape::createRegularPolygon(int sides, double cx, double cy, double radius)
{
    auto s = std::make_unique<PolygonShape>();
    QVector<QPointF> verts;
    for (int i = 0; i < sides; ++i) {
        double angle = -M_PI_2 + i * 2.0 * M_PI / sides;
        verts.append(QPointF(cx + radius * cos(angle), cy + radius * sin(angle)));
    }
    s->setVerticesFromScene(verts);
    return s;
}
