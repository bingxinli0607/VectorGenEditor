#include "ShapeFactory.h"
#include "RectShape.h"
#include "EllipseShape.h"
#include "PolygonShape.h"
#include "PolylineShape.h"
#include "LineShape.h"
#include "TextShape.h"
#include "StarShape.h"
#include "ArrowShape.h"

std::unique_ptr<Shape> ShapeFactory::create(const QString &type)
{
    if (type == "Rect")      return std::make_unique<RectShape>();
    if (type == "Ellipse")   return std::make_unique<EllipseShape>();
    if (type == "Polygon")   return std::make_unique<PolygonShape>();
    if (type == "Polyline")  return std::make_unique<PolylineShape>();
    if (type == "Line")      return std::make_unique<LineShape>();
    if (type == "Text")      return std::make_unique<TextShape>();
    if (type == "Star")      return std::make_unique<StarShape>();
    if (type == "Arrow")     return std::make_unique<ArrowShape>();
    return nullptr;
}

QStringList ShapeFactory::registeredTypes()
{
    return {"Rect", "Ellipse", "Polygon", "Polyline", "Line", "Text", "Star", "Arrow"};
}
