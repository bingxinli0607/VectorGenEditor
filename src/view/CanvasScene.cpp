#include "CanvasScene.h"
#include "ShapeGraphicsItem.h"
#include "controller/SelectionController.h"
#include "controller/DocumentController.h"
#include "model/Document.h"
#include "model/Layer.h"
#include "model/Shape.h"
#include "model/Geometry.h"
#include "model/Workspace.h"

namespace {

bool shapeVisibleInDocument(const Document *doc, const QString &shapeId)
{
    if (!doc) return true;
    for (const auto &layer : doc->layers()) {
        if (!layer->visible()) continue;
        for (const auto &shape : layer->shapes()) {
            if (shape->id() == shapeId)
                return shape->style().visible;
        }
    }
    for (const auto &layer : doc->layers()) {
        for (const auto &shape : layer->shapes()) {
            if (shape->id() == shapeId)
                return false;
        }
    }
    return true;
}

} // namespace

CanvasScene::CanvasScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(Workspace::defaultSceneRect());
    connect(this, &QGraphicsScene::selectionChanged,
            this, &CanvasScene::onSceneSelectionChanged);
}

CanvasScene::~CanvasScene()
{
    delete m_selCtrl;
}

void CanvasScene::setDocument(Document *doc)
{
    if (m_document)
        disconnectDocument();
    m_document = doc;
    if (m_document)
        connectDocument();
    delete m_selCtrl;
    m_selCtrl = nullptr;
    if (m_docCtrl) {
        m_selCtrl = new SelectionController(this, m_docCtrl, this);
    }
    updateSceneRectFromDocument();
}

ShapeGraphicsItem *CanvasScene::itemForShape(const QString &shapeId) const
{
    return m_itemMap.value(shapeId, nullptr);
}

void CanvasScene::connectDocument()
{
    connect(m_document, &Document::shapeAdded,   this, &CanvasScene::onShapeAdded);
    connect(m_document, &Document::shapeRemoved, this, &CanvasScene::onShapeRemoved);
    connect(m_document, &Document::shapeChanged, this, &CanvasScene::onShapeChanged);
    connect(m_document, &Document::documentChanged, this, &CanvasScene::onDocumentChanged);
}

void CanvasScene::disconnectDocument()
{
    if (!m_document) return;
    disconnect(m_document, nullptr, this, nullptr);
}

void CanvasScene::onShapeAdded(const QString &shapeId)
{
    if (!m_document) return;
    Shape *shape = m_document->findShape(shapeId);
    if (!shape) return;

    auto *item = new ShapeGraphicsItem(shape);
    if (m_selCtrl)
        item->setSelectionController(m_selCtrl);
    if (m_docCtrl)
        item->setDocumentController(m_docCtrl);
    item->setVisible(shapeVisibleInDocument(m_document, shapeId));
    addItem(item);
    m_itemMap[shapeId] = item;
    updateSceneRectFromDocument();
}

void CanvasScene::onShapeRemoved(const QString &shapeId)
{
    auto *item = m_itemMap.take(shapeId);
    if (item) {
        if (m_selCtrl) m_selCtrl->deselectAll();
        removeItem(item);
        delete item;
    }
    updateSceneRectFromDocument();
}

void CanvasScene::onShapeChanged(const QString &shapeId)
{
    auto *item = m_itemMap.value(shapeId, nullptr);
    if (item) {
        item->updateFromModel();
        item->setVisible(shapeVisibleInDocument(m_document, shapeId));
    }
    updateSceneRectFromDocument();
}

void CanvasScene::onDocumentChanged()
{
    updateSceneRectFromDocument();
}

void CanvasScene::onSceneSelectionChanged()
{
    // SelectionController handles this via ShapeGraphicsItem::itemChange
}

void CanvasScene::syncLayerVisibility()
{
    if (!m_document) return;
    for (auto it = m_itemMap.constBegin(); it != m_itemMap.constEnd(); ++it)
        it.value()->setVisible(shapeVisibleInDocument(m_document, it.key()));
}

void CanvasScene::syncAllFromDocument()
{
    if (m_selCtrl) m_selCtrl->deselectAll();
    for (auto *item : m_itemMap.values()) {
        removeItem(item);
        delete item;
    }
    m_itemMap.clear();

    if (!m_document) return;
    for (const auto &layer : m_document->layers()) {
        const bool layerVisible = layer->visible();
        for (const auto &shape : layer->shapes()) {
            auto *item = new ShapeGraphicsItem(shape.get());
            if (m_selCtrl)
                item->setSelectionController(m_selCtrl);
            item->setVisible(layerVisible && shape->style().visible);
            addItem(item);
            m_itemMap[shape->id()] = item;
        }
    }
    updateSceneRectFromDocument();
}

QRectF CanvasScene::calculateDocumentShapesBoundingRect(
    const QSet<ShapeGraphicsItem*> *posOverrides) const
{
    QRectF bounds;
    if (!m_document) return bounds;

    for (const auto &layer : m_document->layers()) {
        for (const auto &shape : layer->shapes()) {
            if (!shape->style().visible) continue;

            auto *item = m_itemMap.value(shape->id(), nullptr);
            if (posOverrides && item && posOverrides->contains(item)) {
                bounds |= item->mapToScene(item->localBounds()).boundingRect();
            } else {
                bounds |= Geometry::sceneBounds(*shape);
            }
        }
    }
    return bounds;
}

void CanvasScene::updateSceneRectFromDocument(const QSet<ShapeGraphicsItem*> *posOverrides)
{
    constexpr double kPageScenePad = 200.0;
    constexpr double kShapePad = 300.0;
    constexpr double kPageExpandPad = 200.0;

    const QRectF shapesRect = calculateDocumentShapesBoundingRect(posOverrides);
    QRectF pageTarget = Workspace::pageRectForShapeBounds(shapesRect, kPageExpandPad);
    QRectF sceneTarget = Workspace::sceneRectForPageAndBounds(
        pageTarget, shapesRect, kPageScenePad, kShapePad);

    const bool interacting = m_selCtrl
        && (m_selCtrl->isHandleDragging() || m_selCtrl->isItemDragging());
    if (interacting && m_document) {
        const QRectF curPage = Workspace::sanitizeRect(m_document->pageRect());
        pageTarget = curPage.united(pageTarget);
        sceneTarget = Workspace::sanitizeRect(sceneRect()).united(sceneTarget);
    }

    pageTarget = Workspace::sanitizeRect(pageTarget);
    sceneTarget = Workspace::sanitizeRect(sceneTarget, Workspace::defaultSceneRect());

    if (sceneRect() != sceneTarget)
        setSceneRect(sceneTarget);

    if (m_document && m_document->pageRect() != pageTarget)
        m_document->setPageRect(pageTarget);

    update();
}
