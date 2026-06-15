#ifndef STARSHAPE_H
#define STARSHAPE_H

#include "Shape.h"

class StarShape : public Shape
{
public:
    QString shapeType() const override { return "Star"; }
    std::unique_ptr<Shape> clone() const override;

    QVector<QPointF> vertices() const override { return m_vertices; }
    int points() const { return m_points; }
    double innerRadiusRatio() const { return m_innerRatio; }

    void setParameters(int points, double innerRatio);
    void setLocalVertices(const QVector<QPointF> &local);
    void setVerticesFromScene(const QVector<QPointF> &scene);
    QVector<QPointF> generateVertices(double cx, double cy, double outerR) const;
    void translateBy(double dx, double dy) override;
    void scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds) override;
    void replaceLocalVertices(const QVector<QPointF> &local) override;

protected:
    QJsonObject typeSpecificToJson() const override;
    void typeSpecificFromJson(const QJsonObject &obj) override;

private:
    int    m_points     = 5;
    double m_innerRatio = 0.5;
    QVector<QPointF> m_vertices;
};

#endif // STARSHAPE_H
