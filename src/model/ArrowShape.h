#ifndef ARROWSHAPE_H
#define ARROWSHAPE_H

#include "Shape.h"

class ArrowShape : public Shape
{
public:
    QString shapeType() const override { return "Arrow"; }
    std::unique_ptr<Shape> clone() const override;

    QVector<QPointF> vertices() const override { return m_vertices; }
    double headWidth() const { return m_headWidth; }
    double headLength() const { return m_headLength; }

    void setParameters(double headW, double headL);
    void setStartEnd(const QPointF &start, const QPointF &end);
    QPointF startPoint() const;
    QPointF endPoint() const;
    void translateBy(double dx, double dy) override;
    void scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds) override;
    void replaceLocalVertices(const QVector<QPointF> &local) override;

protected:
    QJsonObject typeSpecificToJson() const override;
    void typeSpecificFromJson(const QJsonObject &obj) override;

private:
    double m_headWidth  = 15.0;
    double m_headLength = 20.0;
    QVector<QPointF> m_vertices;
};

#endif // ARROWSHAPE_H
