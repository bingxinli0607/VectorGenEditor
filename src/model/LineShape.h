#ifndef LINESHAPE_H
#define LINESHAPE_H

#include "Shape.h"

/// Single segment line (two endpoints, local coordinates).
class LineShape : public Shape
{
public:
    QString shapeType() const override { return "Line"; }
    std::unique_ptr<Shape> clone() const override;

    QVector<QPointF> vertices() const override { return m_points; }
    void setEndpointsFromScene(const QPointF &start, const QPointF &end);
    void translateBy(double dx, double dy) override;
    void scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds) override;
    void replaceLocalVertices(const QVector<QPointF> &local) override;

protected:
    QJsonObject typeSpecificToJson() const override;
    void typeSpecificFromJson(const QJsonObject &obj) override;

private:
    void setLocalPoints(const QVector<QPointF> &local);

    QVector<QPointF> m_points;
};

#endif // LINESHAPE_H
