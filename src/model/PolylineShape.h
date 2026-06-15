#ifndef POLYLINESHAPE_H
#define POLYLINESHAPE_H

#include "Shape.h"

class PolylineShape : public Shape
{
public:
    QString shapeType() const override { return "Polyline"; }
    std::unique_ptr<Shape> clone() const override;

    QVector<QPointF> vertices() const override { return m_points; }
    void setLocalPoints(const QVector<QPointF> &local);
    void setPointsFromScene(const QVector<QPointF> &scene);
    void setPoints(const QVector<QPointF> &scene) { setPointsFromScene(scene); }
    bool closed() const { return m_closed; }
    void setClosed(bool c);
    void translateBy(double dx, double dy) override;
    void scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds) override;
    void replaceLocalVertices(const QVector<QPointF> &local) override;

protected:
    QJsonObject typeSpecificToJson() const override;
    void typeSpecificFromJson(const QJsonObject &obj) override;

private:
    QVector<QPointF> m_points;
    bool m_closed = false;
};

#endif // POLYLINESHAPE_H
