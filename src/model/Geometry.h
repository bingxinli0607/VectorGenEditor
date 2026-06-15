#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "Shape.h"
#include <QVector>
#include <QPointF>
#include <QRectF>

namespace Geometry {

QRectF boundsFromPoints(const QVector<QPointF> &points);

QVector<QPointF> toLocalPoints(const QVector<QPointF> &scenePoints, const QPointF &origin);
QVector<QPointF> toScenePoints(const QVector<QPointF> &localPoints, const Transform &t);

/// True when stored vertices look like legacy absolute scene coordinates.
bool looksLikeAbsoluteCoords(const QVector<QPointF> &points, const Transform &t);

QRectF localBounds(const Transform &t);
QRectF rotatedBoundingRect(const Transform &t);

QVector<QPointF> scaleLocalPoints(const QVector<QPointF> &points,
                                  const QRectF &oldBounds,
                                  const QRectF &newBounds);

QPointF snapToGrid(const QPointF &p, double gridSize = 20.0);

/// Axis-aligned scene bounds for export / hit testing.
QRectF sceneBounds(const Shape &shape);

bool intersectsRect(const QRectF &a, const QRectF &b);

} // namespace Geometry

#endif // GEOMETRY_H
