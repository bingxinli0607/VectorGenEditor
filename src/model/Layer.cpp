#include "Layer.h"
#include "Shape.h"
#include "ShapeFactory.h"
#include <QJsonArray>
#include <algorithm>

Layer::Layer(const QString &name) : m_name(name) {}
Layer::~Layer() = default;

QString Layer::name() const              { return m_name; }
void    Layer::setName(const QString &n)  { m_name = n; }
bool    Layer::visible() const            { return m_visible; }
void    Layer::setVisible(bool v)         { m_visible = v; }

const std::vector<std::unique_ptr<Shape>> &Layer::shapes() const { return m_shapes; }
std::vector<std::unique_ptr<Shape>> &Layer::shapes()             { return m_shapes; }

void Layer::addShape(std::unique_ptr<Shape> shape)
{
    m_shapes.push_back(std::move(shape));
}

std::unique_ptr<Shape> Layer::takeShape(const QString &shapeId)
{
    for (auto it = m_shapes.begin(); it != m_shapes.end(); ++it) {
        if ((*it)->id() == shapeId) {
            auto s = std::move(*it);
            m_shapes.erase(it);
            return s;
        }
    }
    return nullptr;
}

Shape *Layer::findShape(const QString &shapeId) const
{
    for (const auto &s : m_shapes) {
        if (s->id() == shapeId) return s.get();
    }
    return nullptr;
}

void Layer::moveShapeUp(const QString &shapeId)
{
    for (size_t i = 1; i < m_shapes.size(); ++i) {
        if (m_shapes[i]->id() == shapeId) {
            std::swap(m_shapes[i], m_shapes[i - 1]);
            return;
        }
    }
}

void Layer::moveShapeDown(const QString &shapeId)
{
    for (size_t i = 0; i + 1 < m_shapes.size(); ++i) {
        if (m_shapes[i]->id() == shapeId) {
            std::swap(m_shapes[i], m_shapes[i + 1]);
            return;
        }
    }
}

void Layer::moveShapeToTop(const QString &shapeId)
{
    for (auto it = m_shapes.begin(); it != m_shapes.end(); ++it) {
        if ((*it)->id() == shapeId) {
            auto shape = std::move(*it);
            m_shapes.erase(it);
            m_shapes.push_back(std::move(shape));
            return;
        }
    }
}

void Layer::moveShapeToBottom(const QString &shapeId)
{
    for (auto it = m_shapes.begin(); it != m_shapes.end(); ++it) {
        if ((*it)->id() == shapeId) {
            auto shape = std::move(*it);
            m_shapes.erase(it);
            m_shapes.insert(m_shapes.begin(), std::move(shape));
            return;
        }
    }
}

QJsonObject Layer::toJson() const
{
    QJsonObject obj;
    obj["name"]    = m_name;
    obj["visible"] = m_visible;
    QJsonArray arr;
    for (const auto &s : m_shapes)
        arr.append(s->toJson());
    obj["shapes"] = arr;
    return obj;
}

std::unique_ptr<Layer> Layer::fromJson(const QJsonObject &obj)
{
    auto layer = std::make_unique<Layer>(obj["name"].toString("Layer"));
    layer->setVisible(obj["visible"].toBool(true));
    QJsonArray arr = obj["shapes"].toArray();
    for (const auto &v : arr) {
        QJsonObject shapeObj = v.toObject();
        auto shape = ShapeFactory::create(shapeObj["type"].toString());
        if (shape) {
            shape->fromJson(shapeObj);
            layer->addShape(std::move(shape));
        }
    }
    return layer;
}
