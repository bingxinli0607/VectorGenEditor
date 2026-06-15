#include "Geometry.h"
#include <QtMath>
#include <algorithm>

namespace Geometry {

QRectF boundsFromPoints(const QVector<QPointF> &points)
{
    if (points.isEmpty()) return QRectF();
    double minX = points[0].x(), minY = points[0].y();
    double maxX = minX, maxY = minY;
    for (const auto &p : points) {
        minX = qMin(minX, p.x());
        minY = qMin(minY, p.y());
        maxX = qMax(maxX, p.x());
        maxY = qMax(maxY, p.y());
    }
    return QRectF(minX, minY, qMax(maxX - minX, 1.0), qMax(maxY - minY, 1.0));
}

QVector<QPointF> toLocalPoints(const QVector<QPointF> &scenePoints, const QPointF &origin)
{
    QVector<QPointF> local;
    local.reserve(scenePoints.size());
    for (const auto &p : scenePoints)
        local.append(QPointF(p.x() - origin.x(), p.y() - origin.y()));
    return local;
}

QPointF mapLocalToScene(const QPointF &local, const Transform &t)
{
    const double cx = t.width / 2.0;
    const double cy = t.height / 2.0;
    const double rad = qDegreesToRadians(t.rotation);
    const double cosA = qCos(rad);
    const double sinA = qSin(rad);
    const double dx = local.x() - cx;
    const double dy = local.y() - cy;
    return QPointF(t.x + cx + dx * cosA - dy * sinA,
                   t.y + cy + dx * sinA + dy * cosA);
}

QVector<QPointF> toScenePoints(const QVector<QPointF> &localPoints, const Transform &t)
{
    QVector<QPointF> scene;
    scene.reserve(localPoints.size());
    for (const auto &p : localPoints)
        scene.append(mapLocalToScene(p, t));
    return scene;
}

bool looksLikeAbsoluteCoords(const QVector<QPointF> &points, const Transform &t)
{
    if (points.isEmpty()) return false;
    const QRectF bounds = boundsFromPoints(points);
    if (bounds.width() > t.width * 1.5 + 10.0 || bounds.height() > t.height * 1.5 + 10.0)
        return true;
    if (bounds.left() > t.x + 5.0 || bounds.top() > t.y + 5.0)
        return true;
    return false;
}

QRectF localBounds(const Transform &t)
{
    return QRectF(0, 0, qMax(t.width, 1.0), qMax(t.height, 1.0));
}

QRectF rotatedBoundingRect(const Transform &t)
{
    const QRectF local = localBounds(t);
    if (qFuzzyIsNull(t.rotation))
        return QRectF(t.x, t.y, local.width(), local.height());

    const double cx = t.x + local.width() / 2.0;
    const double cy = t.y + local.height() / 2.0;
    const double rad = qDegreesToRadians(t.rotation);
    const double cosA = qCos(rad);
    const double sinA = qSin(rad);

    const QPointF corners[4] = {
        QPointF(0, 0), QPointF(local.width(), 0),
        QPointF(local.width(), local.height()), QPointF(0, local.height())
    };

    double minX = 0, minY = 0, maxX = 0, maxY = 0;
    bool first = true;
    for (const auto &c : corners) {
        const double dx = c.x() - local.width() / 2.0;
        const double dy = c.y() - local.height() / 2.0;
        const double sx = cx + dx * cosA - dy * sinA;
        const double sy = cy + dx * sinA + dy * cosA;
        if (first) {
            minX = maxX = sx;
            minY = maxY = sy;
            first = false;
        } else {
            minX = qMin(minX, sx);
            minY = qMin(minY, sy);
            maxX = qMax(maxX, sx);
            maxY = qMax(maxY, sy);
        }
    }
    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

QVector<QPointF> scaleLocalPoints(const QVector<QPointF> &points,
                                  const QRectF &oldBounds,
                                  const QRectF &newBounds)
{
    if (points.isEmpty() || oldBounds.width() < 1e-6 || oldBounds.height() < 1e-6)
        return points;

    const double sx = newBounds.width() / oldBounds.width();
    const double sy = newBounds.height() / oldBounds.height();
    QVector<QPointF> scaled;
    scaled.reserve(points.size());
    for (const auto &p : points) {
        const double nx = newBounds.left() + (p.x() - oldBounds.left()) * sx;
        const double ny = newBounds.top() + (p.y() - oldBounds.top()) * sy;
        scaled.append(QPointF(nx, ny));
    }
    return scaled;
}

QPointF snapToGrid(const QPointF &p, double gridSize)
{
    if (gridSize <= 0.0) return p;
    return QPointF(qRound(p.x() / gridSize) * gridSize,
                   qRound(p.y() / gridSize) * gridSize);
}

QRectF sceneBounds(const Shape &shape)
{
    const QVector<QPointF> verts = shape.vertices();
    if (!verts.isEmpty()) {
        const QVector<QPointF> scene = toScenePoints(verts, shape.transform());
        return boundsFromPoints(scene);
    }
    return rotatedBoundingRect(shape.transform());
}

bool intersectsRect(const QRectF &a, const QRectF &b)
{
    return a.intersects(b);
}

} // namespace Geometry
