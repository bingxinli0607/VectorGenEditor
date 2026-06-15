#include "RectShape.h"

std::unique_ptr<Shape> RectShape::clone() const
{
    auto s = std::make_unique<RectShape>();
    s->setId(id());
    s->setTransform(transform());
    s->setStyle(style());
    return s;
}
