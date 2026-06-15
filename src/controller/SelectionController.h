#ifndef SELECTIONCONTROLLER_H
#define SELECTIONCONTROLLER_H

#include <QObject>
#include <QSet>
#include <QRectF>
#include <QHash>
#include <QPointF>

class CanvasScene;
class ShapeGraphicsItem;
class HandleItem;
class DocumentController;

/// Manages selection state and resize/rotate handles.
/// Forwards completed transforms to DocumentController.
class SelectionController : public QObject
{
    Q_OBJECT

public:
    explicit SelectionController(CanvasScene *scene,
                                  DocumentController *docCtrl,
                                  QObject *parent = nullptr);

    QSet<ShapeGraphicsItem*> selectedItems() const { return m_selected; }
    ShapeGraphicsItem *primarySelected() const;
    bool hasSelection() const { return !m_selected.isEmpty(); }
    int selectionCount() const { return m_selected.size(); }
    bool isSyncingSelection() const { return m_syncGuard; }
    bool isItemSelected(ShapeGraphicsItem *item) const { return m_selected.contains(item); }

    void replaceSelection(ShapeGraphicsItem *item);
    void toggleSelection(ShapeGraphicsItem *item);
    /// @deprecated use replaceSelection — kept for tests
    void select(ShapeGraphicsItem *item) { replaceSelection(item); }
    void selectInRect(const QRectF &sceneRect, bool additive);
    void deselect(ShapeGraphicsItem *item);
    void deselectAll();
    void selectAll();
    void selectByShapeIds(const QStringList &ids);

    void beginItemDrag(ShapeGraphicsItem *leader);
    void previewItemDrag(ShapeGraphicsItem *leader, const QPointF &leaderPos);
    void finishItemDrag(ShapeGraphicsItem *leader);

    // Called by ShapeGraphicsItem after drag/resize/rotate completes
    void onItemMoved(ShapeGraphicsItem *item,
                     const QPointF &oldPos, const QPointF &newPos);
    void onItemResized(ShapeGraphicsItem *item,
                       const QRectF &oldBounds, const QRectF &newBounds,
                       double oldRotation, double newRotation,
                       double oldFontSize = -1.0, double newFontSize = -1.0);
    void onItemRotated(ShapeGraphicsItem *item,
                       double oldAngle, double newAngle);

    void updateHandles(ShapeGraphicsItem *item);
    void setHandleDragging(bool dragging) { m_handleDragging = dragging; }
    bool isHandleDragging() const { return m_handleDragging; }
    bool isItemDragging() const { return m_dragActive; }
    void notifyStatus(const QString &message);

signals:
    void selectionChanged();
    void statusMessage(const QString &message);

private:
    bool isSelectable(ShapeGraphicsItem *item) const;
    void applySelection(const QSet<ShapeGraphicsItem*> &next);
    void createHandles(ShapeGraphicsItem *item);
    void removeAllHandles();

    CanvasScene        *m_scene = nullptr;
    DocumentController *m_docCtrl = nullptr;
    QSet<ShapeGraphicsItem*> m_selected;
    QList<HandleItem*>  m_handles;
    bool m_syncGuard = false;
    bool m_handleDragging = false;

    bool m_dragActive = false;
    ShapeGraphicsItem *m_dragLeader = nullptr;
    QHash<ShapeGraphicsItem*, QPointF> m_dragStartPositions;
};

#endif // SELECTIONCONTROLLER_H
