#ifndef ELLIPSESHAPE_H
#define ELLIPSESHAPE_H

#include "Shape.h"

class EllipseShape : public Shape
{
public:
    QString shapeType() const override { return "Ellipse"; }
    std::unique_ptr<Shape> clone() const override;
};

#endif // ELLIPSESHAPE_H
