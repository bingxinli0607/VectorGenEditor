#ifndef SHAPEGRAPHICSITEM_H
#define SHAPEGRAPHICSITEM_H

#include <QGraphicsItem>
#include <QPainter>

class Shape;
class SelectionController;
class DocumentController;

class ShapeGraphicsItem : public QGraphicsItem
{
public:
    explicit ShapeGraphicsItem(Shape *shape, QGraphicsItem *parent = nullptr);
    ~ShapeGraphicsItem() override;

    Shape *modelShape() const { return m_shape; }
    void setModelShape(Shape *shape);

    void setSelectionController(SelectionController *ctrl) { m_selCtrl = ctrl; }
    void setDocumentController(DocumentController *ctrl) { m_docCtrl = ctrl; }
    DocumentController *documentController() const { return m_docCtrl; }

    QRectF localBounds() const;
    void updateFromModel();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    void drawRectShape(QPainter *painter);
    void drawEllipseShape(QPainter *painter);
    void drawPolygonShape(QPainter *painter);
    void drawPolylineShape(QPainter *painter);
    void drawTextShape(QPainter *painter);
    void drawStarShape(QPainter *painter);
    void drawArrowShape(QPainter *painter);
    void drawSelectionOutline(QPainter *painter);
    void drawHoverOutline(QPainter *painter);

    Shape *m_shape = nullptr;
    SelectionController *m_selCtrl = nullptr;
    DocumentController *m_docCtrl = nullptr;
    QPointF m_dragStartPos;
    QPointF m_dragStartScenePos;
    bool m_dragging = false;
    bool m_selectionGuard = false;
    bool m_hovered = false;
};

#endif // SHAPEGRAPHICSITEM_H
