#ifndef RECTSHAPE_H
#define RECTSHAPE_H

#include "Shape.h"

class RectShape : public Shape
{
public:
    QString shapeType() const override { return "Rect"; }
    std::unique_ptr<Shape> clone() const override;
};

#endif // RECTSHAPE_H
