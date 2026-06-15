#include "Workspace.h"
#include "Geometry.h"
#include <QtMath>

namespace Workspace {

namespace {

QRectF boundsFor(const Shape &shape, const Transform &t)
{
    const QVector<QPointF> verts = shape.vertices();
    if (!verts.isEmpty())
        return Geometry::boundsFromPoints(Geometry::toScenePoints(verts, t));
    return Geometry::rotatedBoundingRect(t);
}

} // namespace

QRectF defaultPageRect()
{
    return QRectF(0, 0, 1200, 800);
}

QRectF defaultSceneRect()
{
    return defaultPageRect().adjusted(-200, -200, 200, 200);
}

namespace {

QRectF validBounds(const QRectF &bounds)
{
    if (!bounds.isValid() || bounds.isEmpty())
        return QRectF();
    const QRectF n = bounds.normalized();
    if (!n.isValid() || n.width() < 1.0 || n.height() < 1.0)
        return QRectF();
    return n;
}

} // namespace

QRectF sanitizeRect(const QRectF &rect, const QRectF &fallback)
{
    const QRectF fb = fallback.isValid() && !fallback.isEmpty()
        ? fallback.normalized() : defaultPageRect();
    const QRectF n = validBounds(rect);
    return n.isEmpty() ? fb : n;
}

QRectF pageRectForShapeBounds(const QRectF &shapesBounds, double margin)
{
    QRectF page = defaultPageRect();
    const QRectF shapes = validBounds(shapesBounds);
    if (!shapes.isEmpty())
        page = page.united(shapes.adjusted(-margin, -margin, margin, margin));
    return sanitizeRect(page);
}

QRectF sceneRectForPageAndBounds(const QRectF &pageRect, const QRectF &shapesBounds,
                                 double pageScenePad, double shapePad)
{
    const QRectF page = sanitizeRect(pageRect);
    QRectF scene = page.adjusted(-pageScenePad, -pageScenePad, pageScenePad, pageScenePad);
    const QRectF shapes = validBounds(shapesBounds);
    if (!shapes.isEmpty())
        scene = scene.united(shapes.adjusted(-shapePad, -shapePad, shapePad, shapePad));
    return sanitizeRect(scene, defaultSceneRect());
}

QRectF rect()
{
    return defaultPageRect();
}

QRectF baseRect()
{
    return defaultPageRect();
}

bool containsPoint(const QPointF &p)
{
    return rect().contains(p);
}

bool containsSceneBounds(const QRectF &bounds)
{
    return rect().contains(bounds);
}

QRectF sceneBoundsForTransform(const Shape &shape, const Transform &t)
{
    return boundsFor(shape, t);
}

QPointF clampTopLeft(const Shape &shape, const QPointF &topLeft, const QRectF &workspace)
{
    Transform t = shape.transform();
    t.x = topLeft.x();
    t.y = topLeft.y();

    QRectF bounds = boundsFor(shape, t);

    double dx = 0.0;
    double dy = 0.0;
    if (bounds.left() < workspace.left())
        dx = workspace.left() - bounds.left();
    if (bounds.top() < workspace.top())
        dy = workspace.top() - bounds.top();
    if (bounds.right() > workspace.right())
        dx = workspace.right() - bounds.right();
    if (bounds.bottom() > workspace.bottom())
        dy = workspace.bottom() - bounds.bottom();

    return QPointF(topLeft.x() + dx, topLeft.y() + dy);
}

bool transformFits(const Shape &shape, const Transform &t, const QRectF &workspace)
{
    return workspace.contains(boundsFor(shape, t));
}

Transform clampTransform(const Shape &shape, const Transform &proposed, const QRectF &workspace)
{
    Transform t = proposed;
    if (t.width < 10.0) t.width = 10.0;
    if (t.height < 10.0) t.height = 10.0;

    while (t.rotation < 0.0) t.rotation += 360.0;
    while (t.rotation >= 360.0) t.rotation -= 360.0;

    QPointF clamped = clampTopLeft(shape, QPointF(t.x, t.y), workspace);
    t.x = clamped.x();
    t.y = clamped.y();

    if (transformFits(shape, t, workspace))
        return t;

    QRectF bounds = boundsFor(shape, t);

    if (qFuzzyIsNull(t.rotation) && shape.vertices().isEmpty()) {
        if (bounds.left() < workspace.left()) {
            const double d = workspace.left() - bounds.left();
            t.x += d;
            t.width = qMax(10.0, t.width - d);
        }
        if (bounds.top() < workspace.top()) {
            const double d = workspace.top() - bounds.top();
            t.y += d;
            t.height = qMax(10.0, t.height - d);
        }
        bounds = boundsFor(shape, t);
        if (bounds.right() > workspace.right())
            t.width = qMax(10.0, t.width - (bounds.right() - workspace.right()));
        if (bounds.bottom() > workspace.bottom())
            t.height = qMax(10.0, t.height - (bounds.bottom() - workspace.bottom()));
        clamped = clampTopLeft(shape, QPointF(t.x, t.y), workspace);
        t.x = clamped.x();
        t.y = clamped.y();
    }

    return t;
}

} // namespace Workspace
