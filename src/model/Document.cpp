#include "Document.h"
#include "Layer.h"
#include "Shape.h"
#include "ShapeFactory.h"
#include "Workspace.h"
#include <QJsonArray>

Document::Document(QObject *parent) : QObject(parent)
{
    m_workspaceRect = Workspace::defaultPageRect();
    m_layers.push_back(std::make_unique<Layer>("Layer 1"));
}

Document::~Document() = default;

// --- Layers ---

const std::vector<std::unique_ptr<Layer>> &Document::layers() const { return m_layers; }
std::vector<std::unique_ptr<Layer>> &Document::layers()             { return m_layers; }

Layer *Document::currentLayer() const
{
    if (m_currentLayerIndex < 0 || static_cast<size_t>(m_currentLayerIndex) >= m_layers.size())
        return nullptr;
    return m_layers[m_currentLayerIndex].get();
}

int Document::currentLayerIndex() const { return m_currentLayerIndex; }

void Document::setCurrentLayer(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < m_layers.size()) {
        m_currentLayerIndex = index;
        emit layerChanged();
    }
}

Layer *Document::addLayer(const QString &name)
{
    m_layers.push_back(std::make_unique<Layer>(name));
    m_currentLayerIndex = static_cast<int>(m_layers.size()) - 1;
    emit layerChanged();
    return m_layers.back().get();
}

void Document::removeLayer(int index)
{
    if (m_layers.size() <= 1) return;
    if (index < 0 || static_cast<size_t>(index) >= m_layers.size()) return;
    for (const auto &shape : m_layers[index]->shapes())
        emit shapeRemoved(shape->id());
    m_layers.erase(m_layers.begin() + index);
    if (m_currentLayerIndex >= static_cast<int>(m_layers.size()))
        m_currentLayerIndex = static_cast<int>(m_layers.size()) - 1;
    emit layerChanged();
    emit documentChanged();
}

Layer *Document::layerAt(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= m_layers.size()) return nullptr;
    return m_layers[index].get();
}

int Document::layerCount() const { return static_cast<int>(m_layers.size()); }

// --- Shape operations ---

void Document::addShape(std::unique_ptr<Shape> shape)
{
    if (!shape || !currentLayer()) return;
    QString id = shape->id();
    currentLayer()->addShape(std::move(shape));
    emit shapeAdded(id);
    emit documentChanged();
}

std::unique_ptr<Shape> Document::takeShape(const QString &shapeId)
{
    for (auto &layer : m_layers) {
        auto s = layer->takeShape(shapeId);
        if (s) {
            emit shapeRemoved(shapeId);
            emit documentChanged();
            return s;
        }
    }
    return nullptr;
}

Shape *Document::findShape(const QString &shapeId) const
{
    for (const auto &layer : m_layers) {
        auto s = layer->findShape(shapeId);
        if (s) return s;
    }
    return nullptr;
}

void Document::moveShapeUp(const QString &shapeId)
{
    for (auto &layer : m_layers) {
        layer->moveShapeUp(shapeId);
    }
    emit documentChanged();
}

void Document::moveShapeDown(const QString &shapeId)
{
    for (auto &layer : m_layers) {
        layer->moveShapeDown(shapeId);
    }
    emit documentChanged();
}

void Document::moveShapeToTop(const QString &shapeId)
{
    for (auto &layer : m_layers) {
        layer->moveShapeToTop(shapeId);
    }
    emit documentChanged();
}

void Document::moveShapeToBottom(const QString &shapeId)
{
    for (auto &layer : m_layers) {
        layer->moveShapeToBottom(shapeId);
    }
    emit documentChanged();
}

void Document::clear()
{
    m_layers.clear();
    m_layers.push_back(std::make_unique<Layer>("Layer 1"));
    m_currentLayerIndex = 0;
    m_workspaceRect = Workspace::defaultPageRect();
    emit layerChanged();
    emit documentChanged();
}

void Document::replaceFrom(const Document &other)
{
    m_layers.clear();
    for (const auto &layer : other.layers()) {
        auto newLayer = std::make_unique<Layer>(layer->name());
        newLayer->setVisible(layer->visible());
        for (const auto &shape : layer->shapes())
            newLayer->addShape(shape->clone());
        m_layers.push_back(std::move(newLayer));
    }
    if (m_layers.empty())
        m_layers.push_back(std::make_unique<Layer>("Layer 1"));
    m_currentLayerIndex = other.currentLayerIndex();
    if (m_currentLayerIndex >= layerCount())
        m_currentLayerIndex = 0;
    m_workspaceRect = other.workspaceRect();
    emit layerChanged();
    emit documentChanged();
}

// --- JSON ---

QJsonObject Document::toJson() const
{
    QJsonObject obj;
    obj["formatVersion"] = 1;
    obj["appName"]       = "VectorGenEditor";
    QJsonArray layersArr;
    for (const auto &layer : m_layers)
        layersArr.append(layer->toJson());
    obj["layers"] = layersArr;
    obj["currentLayerIndex"] = m_currentLayerIndex;
    QJsonObject ws;
    ws["x"] = m_workspaceRect.x();
    ws["y"] = m_workspaceRect.y();
    ws["width"] = m_workspaceRect.width();
    ws["height"] = m_workspaceRect.height();
    obj["workspaceRect"] = ws;
    return obj;
}

std::unique_ptr<Document> Document::fromJson(const QJsonObject &obj, QObject *parent)
{
    auto doc = std::make_unique<Document>(parent);
    doc->m_layers.clear();
    QJsonArray layersArr = obj["layers"].toArray();
    for (const auto &v : layersArr) {
        QJsonObject layerObj = v.toObject();
        doc->m_layers.push_back(Layer::fromJson(layerObj));
    }
    if (doc->m_layers.empty())
        doc->m_layers.push_back(std::make_unique<Layer>("Layer 1"));
    doc->m_currentLayerIndex = obj["currentLayerIndex"].toInt(0);
    if (doc->m_currentLayerIndex >= static_cast<int>(doc->m_layers.size()))
        doc->m_currentLayerIndex = 0;
    if (obj.contains("workspaceRect")) {
        QJsonObject ws = obj["workspaceRect"].toObject();
        doc->m_workspaceRect = QRectF(
            ws["x"].toDouble(0),
            ws["y"].toDouble(0),
            ws["width"].toDouble(1200),
            ws["height"].toDouble(800));
    } else {
        doc->m_workspaceRect = Workspace::defaultPageRect();
    }
    return doc;
}
