#include "EllipseShape.h"

std::unique_ptr<Shape> EllipseShape::clone() const
{
    auto s = std::make_unique<EllipseShape>();
    s->setId(id());
    s->setTransform(transform());
    s->setStyle(style());
    return s;
}
