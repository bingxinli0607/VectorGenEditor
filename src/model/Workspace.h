#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "Shape.h"
#include <QRectF>
#include <QPointF>

/// White drawing page and scrollable scene helpers.
namespace Workspace {

/// Default visible white page (pageRect).
QRectF defaultPageRect();
/// Initial scene scroll area: pageRect padded by 200px on each side.
QRectF defaultSceneRect();
/// pageRect from default page united with shape bounds (shrinks when shapes move back).
QRectF pageRectForShapeBounds(const QRectF &shapesBounds, double margin = 200.0);
QRectF sceneRectForPageAndBounds(const QRectF &pageRect, const QRectF &shapesBounds,
                                 double pageScenePad = 200.0, double shapePad = 300.0);
/// Clamp invalid/empty rects to sane defaults.
QRectF sanitizeRect(const QRectF &rect, const QRectF &fallback = QRectF());

QRectF rect();
QRectF baseRect();

bool containsPoint(const QPointF &p);
bool containsSceneBounds(const QRectF &bounds);

QRectF sceneBoundsForTransform(const Shape &shape, const Transform &t);
QPointF clampTopLeft(const Shape &shape, const QPointF &topLeft,
                     const QRectF &workspace = rect());
Transform clampTransform(const Shape &shape, const Transform &proposed,
                         const QRectF &workspace = rect());
bool transformFits(const Shape &shape, const Transform &t,
                   const QRectF &workspace = rect());

} // namespace Workspace

#endif // WORKSPACE_H
