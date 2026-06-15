#include "ArrowShape.h"
#include "Geometry.h"
#include <QJsonArray>
#include <QtMath>

std::unique_ptr<Shape> ArrowShape::clone() const
{
    auto s = std::make_unique<ArrowShape>();
    s->setId(id());
    s->setTransform(transform());
    s->setStyle(style());
    s->m_headWidth  = m_headWidth;
    s->m_headLength = m_headLength;
    s->m_vertices   = m_vertices;
    return s;
}

void ArrowShape::setParameters(double headW, double headL)
{
    m_headWidth  = qMax(headW, 1.0);
    m_headLength = qMax(headL, 1.0);
}

void ArrowShape::setStartEnd(const QPointF &start, const QPointF &end)
{
    m_vertices.clear();
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double len = qSqrt(dx*dx + dy*dy);
    if (len < 1.0) return;

    double ux = dx / len, uy = dy / len;
    double nx = -uy, ny = ux;

    QPointF headBase(end.x() - ux * m_headLength, end.y() - uy * m_headLength);
    double hw = m_headWidth / 2.0;
    double halfShaft = 2.0;

    QVector<QPointF> scene;
    scene.append(QPointF(start.x() + nx * halfShaft, start.y() + ny * halfShaft));
    scene.append(QPointF(headBase.x() + nx * halfShaft, headBase.y() + ny * halfShaft));
    scene.append(QPointF(headBase.x() + nx * hw, headBase.y() + ny * hw));
    scene.append(end);
    scene.append(QPointF(headBase.x() - nx * hw, headBase.y() - ny * hw));
    scene.append(QPointF(headBase.x() - nx * halfShaft, headBase.y() - ny * halfShaft));
    scene.append(QPointF(start.x() - nx * halfShaft, start.y() - ny * halfShaft));

    const QRectF b = Geometry::boundsFromPoints(scene);
    Transform t = transform();
    t.x = b.x();
    t.y = b.y();
    t.width  = qMax(b.width(), 1.0);
    t.height = qMax(b.height(), 1.0);
    setTransform(t);
    m_vertices = Geometry::toLocalPoints(scene, QPointF(t.x, t.y));
}

void ArrowShape::translateBy(double dx, double dy)
{
    Shape::translateBy(dx, dy);
}

void ArrowShape::scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds)
{
    m_vertices = Geometry::scaleLocalPoints(m_vertices, oldBounds, newBounds);
}

void ArrowShape::replaceLocalVertices(const QVector<QPointF> &local)
{
    m_vertices = local;
    const QRectF b = Geometry::boundsFromPoints(local);
    Transform t = transform();
    t.width  = qMax(b.width(), 1.0);
    t.height = qMax(b.height(), 1.0);
    setTransform(t);
}

QPointF ArrowShape::startPoint() const
{
    if (m_vertices.size() < 2) return QPointF();
    const Transform t = transform();
    const QPointF local = (m_vertices[0] + m_vertices[6]) / 2.0;
    return QPointF(t.x + local.x(), t.y + local.y());
}

QPointF ArrowShape::endPoint() const
{
    const QPointF local = m_vertices.value(3, QPointF());
    const Transform t = transform();
    return QPointF(t.x + local.x(), t.y + local.y());
}

QJsonObject ArrowShape::typeSpecificToJson() const
{
    QJsonObject obj;
    obj["coordSpace"] = "local";
    obj["headWidth"]  = m_headWidth;
    obj["headLength"] = m_headLength;
    QJsonArray arr;
    for (const auto &p : m_vertices) {
        QJsonObject pt;
        pt["x"] = p.x(); pt["y"] = p.y();
        arr.append(pt);
    }
    obj["vertices"] = arr;
    return obj;
}

void ArrowShape::typeSpecificFromJson(const QJsonObject &obj)
{
    m_headWidth  = obj["headWidth"].toDouble(15.0);
    m_headLength = obj["headLength"].toDouble(20.0);
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
        m_vertices = verts;
        const QRectF b = Geometry::boundsFromPoints(verts);
        Transform t = transform();
        t.width  = qMax(b.width(), 1.0);
        t.height = qMax(b.height(), 1.0);
        setTransform(t);
    } else {
        const QRectF b = Geometry::boundsFromPoints(verts);
        Transform t = transform();
        t.x = b.x();
        t.y = b.y();
        t.width  = qMax(b.width(), 1.0);
        t.height = qMax(b.height(), 1.0);
        setTransform(t);
        m_vertices = Geometry::toLocalPoints(verts, QPointF(t.x, t.y));
    }
}
