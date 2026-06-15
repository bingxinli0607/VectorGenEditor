#ifndef HANDLEITEM_H
#define HANDLEITEM_H

#include <QGraphicsRectItem>
#include <QVector>
#include <QPointF>
#include "model/Shape.h"

enum class HandleRole { Move, ResizeTL, ResizeTR, ResizeBL, ResizeBR,
                        ResizeT, ResizeB, ResizeL, ResizeR, Rotate };

class SelectionController;
class ShapeGraphicsItem;
class TextShape;

class HandleItem : public QGraphicsRectItem
{
public:
    HandleItem(HandleRole role, QGraphicsItem *parent = nullptr);

    HandleRole role() const { return m_role; }
    void setSelectionController(SelectionController *ctrl) { m_selCtrl = ctrl; }
    void setTargetShape(ShapeGraphicsItem *target) { m_target = target; }
    ShapeGraphicsItem *targetShape() const { return m_target; }

    static QString roleName(HandleRole role);
    static QPointF anchorForRole(HandleRole role, const QRectF &bounds);
    void updatePosition(const QRectF &bounds);

    QPainterPath shape() const override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    Transform computeTransform(const QPointF &localPos, bool uniform) const;
    void applyPreview(const Transform &t);
    void applyTextFontScale(TextShape *txt, const Transform &clamped) const;
    double scaledTextFontSize(const Transform &clamped) const;
    static bool isTextResizeRole(HandleRole role);

    HandleRole m_role;
    SelectionController *m_selCtrl = nullptr;
    ShapeGraphicsItem *m_target = nullptr;
    Transform m_startTransform;
    QVector<QPointF> m_startVertices;
    double m_startFontSize = 24.0;
    bool m_dragging = false;

    static QRectF handleRect(const QPointF &center);
    static QColor colorForRole(HandleRole role);
};

#endif // HANDLEITEM_H
