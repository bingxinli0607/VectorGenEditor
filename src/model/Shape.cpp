#include "Shape.h"
#include <QUuid>

// --- Transform ---

QJsonObject Transform::toJson() const
{
    QJsonObject obj;
    obj["x"] = x;
    obj["y"] = y;
    obj["width"] = width;
    obj["height"] = height;
    obj["rotation"] = rotation;
    return obj;
}

Transform Transform::fromJson(const QJsonObject &obj)
{
    Transform t;
    t.x        = obj["x"].toDouble(0.0);
    t.y        = obj["y"].toDouble(0.0);
    t.width    = obj["width"].toDouble(100.0);
    t.height   = obj["height"].toDouble(100.0);
    t.rotation = obj["rotation"].toDouble(0.0);
    return t;
}

QRectF Transform::boundingRect() const
{
    return QRectF(x, y, width, height);
}

// --- Style ---

QJsonObject Style::toJson() const
{
    QJsonObject obj;
    obj["fillColor"]   = fillColor;
    obj["strokeColor"] = strokeColor;
    obj["strokeWidth"] = strokeWidth;
    obj["visible"]     = visible;
    obj["lineStyle"]   = lineStyle;
    return obj;
}

Style Style::fromJson(const QJsonObject &obj)
{
    Style s;
    s.fillColor   = obj["fillColor"].toString("#4A90D9");
    s.strokeColor = obj["strokeColor"].toString("#333333");
    s.strokeWidth = obj["strokeWidth"].toDouble(2.0);
    s.visible     = obj["visible"].toBool(true);
    s.lineStyle   = obj["lineStyle"].toString("solid");
    return s;
}

// --- Shape ---

Shape::Shape()  { m_id = generateShapeId(); }
Shape::~Shape() = default;

QString Shape::id() const              { return m_id; }
void    Shape::setId(const QString &id) { m_id = id; }

Transform Shape::transform() const         { return m_transform; }
void      Shape::setTransform(const Transform &t) { m_transform = t; }
QRectF    Shape::boundingRect() const      { return m_transform.boundingRect(); }

Style Shape::style() const           { return m_style; }
void  Shape::setStyle(const Style &s) { m_style = s; }

QJsonObject Shape::toJson() const
{
    QJsonObject obj;
    obj["id"]           = m_id;
    obj["type"]         = shapeType();
    obj["transform"]    = m_transform.toJson();
    obj["style"]        = m_style.toJson();
    QJsonObject extra = typeSpecificToJson();
    if (!extra.isEmpty())
        obj["typeSpecific"] = extra;
    return obj;
}

void Shape::fromJson(const QJsonObject &obj)
{
    m_id = obj["id"].toString();
    m_transform = Transform::fromJson(obj["transform"].toObject());
    m_style     = Style::fromJson(obj["style"].toObject());
    if (obj.contains("typeSpecific"))
        typeSpecificFromJson(obj["typeSpecific"].toObject());
}

void Shape::translateBy(double dx, double dy)
{
    Transform t = m_transform;
    t.x += dx;
    t.y += dy;
    m_transform = t;
}

void Shape::scaleLocalGeometry(const QRectF &, const QRectF &) {}

void Shape::replaceLocalVertices(const QVector<QPointF> &) {}

// --- Helpers ---

QString generateShapeId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
