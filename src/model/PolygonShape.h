#ifndef POLYGONSHAPE_H
#define POLYGONSHAPE_H

#include "Shape.h"

class PolygonShape : public Shape
{
public:
    QString shapeType() const override { return "Polygon"; }
    std::unique_ptr<Shape> clone() const override;

    QVector<QPointF> vertices() const override { return m_vertices; }
    void setLocalVertices(const QVector<QPointF> &local);
    void setVerticesFromScene(const QVector<QPointF> &scene);
    void translateBy(double dx, double dy) override;
    void scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds) override;
    void replaceLocalVertices(const QVector<QPointF> &local) override;
    int vertexCount() const { return m_vertices.size(); }

    static std::unique_ptr<PolygonShape> createTriangle(double cx, double cy, double size);
    static std::unique_ptr<PolygonShape> createRegularPolygon(int sides, double cx, double cy, double radius);

protected:
    QJsonObject typeSpecificToJson() const override;
    void typeSpecificFromJson(const QJsonObject &obj) override;

private:
    QVector<QPointF> m_vertices;
};

#endif // POLYGONSHAPE_H
