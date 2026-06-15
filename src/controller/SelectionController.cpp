#include "SelectionController.h"
#include "view/CanvasScene.h"
#include "view/ShapeGraphicsItem.h"
#include "view/HandleItem.h"
#include "controller/DocumentController.h"
#include "model/Document.h"
#include "model/Shape.h"
#include "model/Geometry.h"
#include "model/Workspace.h"
#include "command/MoveShapesCommand.h"

#include <QVector>

SelectionController::SelectionController(CanvasScene *scene,
                                           DocumentController *docCtrl,
                                           QObject *parent)
    : QObject(parent), m_scene(scene), m_docCtrl(docCtrl)
{
}

ShapeGraphicsItem *SelectionController::primarySelected() const
{
    return m_selected.isEmpty() ? nullptr : *m_selected.begin();
}

bool SelectionController::isSelectable(ShapeGraphicsItem *item) const
{
    if (!item || !item->isVisible()) return false;
    Shape *shape = item->modelShape();
    return shape && shape->style().visible;
}

void SelectionController::applySelection(const QSet<ShapeGraphicsItem*> &next)
{
    const QSet<ShapeGraphicsItem*> old = m_selected;

    m_syncGuard = true;
    m_selected = next;

    for (auto *item : old) {
        if (!next.contains(item)) {
            item->setSelected(false);
            item->update();
        }
    }
    for (auto *item : next) {
        if (!old.contains(item)) {
            item->setSelected(true);
            item->update();
        }
    }
    m_syncGuard = false;

    removeAllHandles();
    if (next.size() == 1)
        createHandles(*next.begin());
    emit selectionChanged();
}

void SelectionController::replaceSelection(ShapeGraphicsItem *item)
{
    if (!isSelectable(item)) return;
    applySelection({item});
}

void SelectionController::toggleSelection(ShapeGraphicsItem *item)
{
    if (!isSelectable(item)) return;
    QSet<ShapeGraphicsItem*> next = m_selected;
    if (next.contains(item))
        next.remove(item);
    else
        next.insert(item);
    applySelection(next);
}

void SelectionController::selectInRect(const QRectF &sceneRect, bool additive)
{
    QSet<ShapeGraphicsItem*> hits;
    const QList<QGraphicsItem*> found = m_scene->items(sceneRect, Qt::IntersectsItemShape);
    for (auto *gi : found) {
        auto *sgi = dynamic_cast<ShapeGraphicsItem*>(gi);
        if (isSelectable(sgi))
            hits.insert(sgi);
    }

    QSet<ShapeGraphicsItem*> next = additive ? m_selected : QSet<ShapeGraphicsItem*>();
    for (auto *h : hits)
        next.insert(h);
    applySelection(next);
}

void SelectionController::deselect(ShapeGraphicsItem *item)
{
    if (!item || !m_selected.contains(item)) return;
    QSet<ShapeGraphicsItem*> next = m_selected;
    next.remove(item);
    applySelection(next);
}

void SelectionController::deselectAll()
{
    applySelection({});
}

void SelectionController::selectAll()
{
    QSet<ShapeGraphicsItem*> all;
    for (auto *item : m_scene->shapeItems()) {
        if (isSelectable(item))
            all.insert(item);
    }
    applySelection(all);
}

void SelectionController::selectByShapeIds(const QStringList &ids)
{
    QSet<ShapeGraphicsItem*> next;
    for (const QString &id : ids) {
        auto *item = m_scene->itemForShape(id);
        if (isSelectable(item))
            next.insert(item);
    }
    applySelection(next);
}

void SelectionController::beginItemDrag(ShapeGraphicsItem *leader)
{
    m_dragActive = true;
    m_dragLeader = leader;
    m_dragStartPositions.clear();
    for (auto *item : m_selected)
        m_dragStartPositions[item] = item->pos();
}

void SelectionController::previewItemDrag(ShapeGraphicsItem *leader, const QPointF &leaderPos)
{
    if (!m_dragActive || !leader || !m_dragStartPositions.contains(leader))
        return;

    const QPointF delta = leaderPos - m_dragStartPositions[leader];

    for (auto it = m_dragStartPositions.constBegin(); it != m_dragStartPositions.constEnd(); ++it) {
        ShapeGraphicsItem *item = it.key();
        if (!item->modelShape()) continue;

        QPointF target = Geometry::snapToGrid(it.value() + delta);
        item->setPos(target);
    }

    m_scene->updateSceneRectFromDocument(&m_selected);

    const QRectF workspace = m_docCtrl && m_docCtrl->document()
        ? m_docCtrl->document()->workspaceRect()
        : Workspace::rect();

    for (auto it = m_dragStartPositions.constBegin(); it != m_dragStartPositions.constEnd(); ++it) {
        ShapeGraphicsItem *item = it.key();
        if (!item->modelShape()) continue;

        QPointF target = item->pos();
        target = Workspace::clampTopLeft(*item->modelShape(), target, workspace);
        item->setPos(target);
    }

    if (selectionCount() == 1)
        updateHandles(leader);
}

void SelectionController::finishItemDrag(ShapeGraphicsItem *leader)
{
    if (!m_dragActive || !leader || !m_docCtrl) {
        m_dragActive = false;
        m_dragLeader = nullptr;
        m_dragStartPositions.clear();
        return;
    }

    QVector<ShapeMoveEntry> moves;
    for (auto it = m_dragStartPositions.constBegin(); it != m_dragStartPositions.constEnd(); ++it) {
        ShapeGraphicsItem *item = it.key();
        if (!item->modelShape()) continue;
        const QPointF oldPos = it.value();
        const QPointF newPos = item->pos();
        if (oldPos == newPos) continue;
        moves.append({item->modelShape()->id(), oldPos, newPos});
    }

    m_dragActive = false;
    m_dragLeader = nullptr;
    m_dragStartPositions.clear();

    if (!moves.isEmpty())
        m_docCtrl->updateShapesPosition(moves);

    for (auto *item : m_selected)
        item->updateFromModel();

    if (m_scene)
        m_scene->updateSceneExtents();
}

void SelectionController::onItemMoved(ShapeGraphicsItem *item,
                                       const QPointF &oldPos, const QPointF &newPos)
{
    if (!item || !item->modelShape()) return;
    ShapeMoveEntry entry{item->modelShape()->id(), oldPos, newPos};
    m_docCtrl->updateShapesPosition({entry});
    updateHandles(item);
}

void SelectionController::onItemResized(ShapeGraphicsItem *item,
                                         const QRectF &oldBounds, const QRectF &newBounds,
                                         double oldRotation, double newRotation,
                                         double oldFontSize, double newFontSize)
{
    if (!item || !item->modelShape()) return;
    m_docCtrl->updateShapeGeometry(item->modelShape()->id(),
                                    oldBounds.x(), oldBounds.y(),
                                    oldBounds.width(), oldBounds.height(),
                                    oldRotation,
                                    newBounds.x(), newBounds.y(),
                                    newBounds.width(), newBounds.height(),
                                    newRotation,
                                    oldFontSize, newFontSize);
    updateHandles(item);
    m_scene->updateSceneExtents();
}

void SelectionController::onItemRotated(ShapeGraphicsItem *item,
                                         double oldAngle, double newAngle)
{
    if (!item || !item->modelShape()) return;
    m_docCtrl->updateShapeRotation(item->modelShape()->id(), oldAngle, newAngle);
    updateHandles(item);
}

void SelectionController::notifyStatus(const QString &message)
{
    emit statusMessage(message);
}

void SelectionController::createHandles(ShapeGraphicsItem *item)
{
    removeAllHandles();
    if (!item || !item->modelShape()) return;

    const QRectF bounds = item->localBounds();

    const QVector<HandleRole> roles = {
        HandleRole::ResizeTL, HandleRole::ResizeTR,
        HandleRole::ResizeBL, HandleRole::ResizeBR,
        HandleRole::ResizeT, HandleRole::ResizeB,
        HandleRole::ResizeL, HandleRole::ResizeR,
        HandleRole::Rotate
    };

    for (auto role : roles) {
        auto *h = new HandleItem(role, nullptr);
        h->setSelectionController(this);
        h->setTargetShape(item);
        m_scene->addItem(h);
        h->updatePosition(bounds);
        m_handles.append(h);
    }
}

void SelectionController::removeAllHandles()
{
    for (auto *h : m_handles)
        delete h;
    m_handles.clear();
}

void SelectionController::updateHandles(ShapeGraphicsItem *item)
{
    if (!item || m_handles.isEmpty() || selectionCount() != 1) return;
    const QRectF bounds = item->localBounds();
    for (auto *h : m_handles)
        h->updatePosition(bounds);
}
