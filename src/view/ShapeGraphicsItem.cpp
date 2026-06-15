#include "ShapeGraphicsItem.h"
#include "controller/SelectionController.h"
#include "model/Shape.h"
#include "model/Geometry.h"
#include "model/RectShape.h"
#include "model/EllipseShape.h"
#include "model/PolygonShape.h"
#include "model/PolylineShape.h"
#include "model/LineShape.h"
#include "model/TextShape.h"
#include "model/StarShape.h"
#include "model/ArrowShape.h"
#include "model/Workspace.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QInputDialog>
#include "controller/DocumentController.h"
#include "model/Document.h"
#include "command/ChangeTextCommand.h"

ShapeGraphicsItem::ShapeGraphicsItem(Shape *shape, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_shape(shape)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
    setAcceptHoverEvents(true);
    updateFromModel();
}

ShapeGraphicsItem::~ShapeGraphicsItem() = default;

void ShapeGraphicsItem::setModelShape(Shape *shape)
{
    m_shape = shape;
    updateFromModel();
}

QRectF ShapeGraphicsItem::localBounds() const
{
    if (!m_shape) return QRectF();
    return Geometry::localBounds(m_shape->transform());
}

void ShapeGraphicsItem::updateFromModel()
{
    if (!m_shape) return;
    Transform t = m_shape->transform();
    setPos(t.x, t.y);
    setTransformOriginPoint(QPointF(t.width / 2.0, t.height / 2.0));
    setRotation(t.rotation);
    const bool interactive = m_shape->style().visible;
    setAcceptedMouseButtons(interactive ? Qt::LeftButton : Qt::NoButton);
    setFlag(QGraphicsItem::ItemIsSelectable, interactive);
    prepareGeometryChange();
    update();
}

QRectF ShapeGraphicsItem::boundingRect() const
{
    return localBounds();
}

QPainterPath ShapeGraphicsItem::shape() const
{
    QPainterPath path;
    path.addRect(localBounds());
    return path;
}

QVariant ShapeGraphicsItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedChange && m_selCtrl && !m_selectionGuard) {
        if (m_selCtrl->isSyncingSelection())
            return value;
        const bool shouldSelect = m_selCtrl->isItemSelected(this);
        if (value.toBool() != shouldSelect)
            return shouldSelect;
    }

    if (change == ItemPositionChange && m_shape && m_dragging) {
        QPointF newPos = value.toPointF();
        const QRectF workspace = m_docCtrl && m_docCtrl->document()
            ? m_docCtrl->document()->workspaceRect()
            : Workspace::rect();
        newPos = Workspace::clampTopLeft(*m_shape, newPos, workspace);
        return newPos;
    }

    return QGraphicsItem::itemChange(change, value);
}

void ShapeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_selCtrl && m_selCtrl->isHandleDragging()) {
        event->ignore();
        return;
    }

    if (event->button() == Qt::LeftButton && m_selCtrl) {
        const bool mod = event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
        m_selectionGuard = true;
        if (mod) {
            m_selCtrl->toggleSelection(this);
        } else if (!m_selCtrl->isItemSelected(this)) {
            m_selCtrl->replaceSelection(this);
        }
        m_selectionGuard = false;
    }

    if (!m_selCtrl || !m_selCtrl->isItemSelected(this)) {
        event->ignore();
        return;
    }

    m_dragStartPos = pos();
    m_dragStartScenePos = event->scenePos();
    m_dragging = true;
    m_selCtrl->beginItemDrag(this);
    event->accept();
}

void ShapeGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_dragging || !(event->buttons() & Qt::LeftButton)) {
        event->ignore();
        return;
    }

    const QPointF delta = event->scenePos() - m_dragStartScenePos;
    const QPointF snapped = Geometry::snapToGrid(m_dragStartPos + delta);
    if (m_selCtrl)
        m_selCtrl->previewItemDrag(this, snapped);
    event->accept();
}

void ShapeGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragging) {
        m_dragging = false;
        if (m_selCtrl)
            m_selCtrl->finishItemDrag(this);
    }
    event->accept();
}

void ShapeGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    auto *txt = dynamic_cast<TextShape*>(m_shape);
    if (txt) {
        bool ok = false;
        const QString newText = QInputDialog::getMultiLineText(
            nullptr, QStringLiteral("编辑文本"), QStringLiteral("内容:"),
            txt->text(), &ok);
        if (ok && m_docCtrl) {
            TextProps oldP{txt->text(), txt->fontFamily(), txt->fontSize(), txt->style().strokeColor};
            TextProps newP{newText, txt->fontFamily(), txt->fontSize(), txt->style().strokeColor};
            m_docCtrl->undoStack()->push(
                new ChangeTextCommand(m_docCtrl->document(), txt->id(), oldP, newP));
        }
        event->accept();
        return;
    }
    QGraphicsItem::mouseDoubleClickEvent(event);
}

void ShapeGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    m_hovered = true;
    update();
    QGraphicsItem::hoverEnterEvent(event);
}

void ShapeGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_hovered = false;
    update();
    QGraphicsItem::hoverLeaveEvent(event);
}

void ShapeGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    if (!m_shape || !m_shape->style().visible) return;

    painter->setRenderHint(QPainter::Antialiasing);

    QString type = m_shape->shapeType();
    if (type == "Rect")       drawRectShape(painter);
    else if (type == "Ellipse")  drawEllipseShape(painter);
    else if (type == "Polygon")  drawPolygonShape(painter);
    else if (type == "Polyline") drawPolylineShape(painter);
    else if (type == "Line")     drawPolylineShape(painter);
    else if (type == "Text")     drawTextShape(painter);
    else if (type == "Star")     drawStarShape(painter);
    else if (type == "Arrow")    drawArrowShape(painter);

    const bool selected = m_selCtrl && m_selCtrl->isItemSelected(this);
    if (selected || (option && (option->state & QStyle::State_Selected)))
        drawSelectionOutline(painter);
    else if (m_hovered)
        drawHoverOutline(painter);
}

void ShapeGraphicsItem::drawSelectionOutline(QPainter *painter)
{
    QPen pen(QColor(70, 130, 220), 1.5, Qt::SolidLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(localBounds());
}

void ShapeGraphicsItem::drawHoverOutline(QPainter *painter)
{
    QPen pen(QColor(47, 128, 237, 160), 1.0, Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(localBounds());
}

void ShapeGraphicsItem::drawRectShape(QPainter *painter)
{
    Style s = m_shape->style();
    Transform t = m_shape->transform();
    QRectF r(0, 0, t.width, t.height);

    painter->setBrush(s.fillColor != "none" ? QColor(s.fillColor) : Qt::NoBrush);
    QPen pen(QColor(s.strokeColor), s.strokeWidth);
    if (s.lineStyle == "dash")
        pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->drawRect(r);
}

void ShapeGraphicsItem::drawEllipseShape(QPainter *painter)
{
    Style s = m_shape->style();
    Transform t = m_shape->transform();
    QRectF r(0, 0, t.width, t.height);

    painter->setBrush(s.fillColor != "none" ? QColor(s.fillColor) : Qt::NoBrush);
    QPen pen(QColor(s.strokeColor), s.strokeWidth);
    if (s.lineStyle == "dash")
        pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->drawEllipse(r);
}

void ShapeGraphicsItem::drawPolygonShape(QPainter *painter)
{
    auto *poly = dynamic_cast<PolygonShape*>(m_shape);
    if (!poly) return;
    Style s = m_shape->style();
    QVector<QPointF> verts = poly->vertices();
    if (verts.isEmpty()) return;

    painter->setBrush(s.fillColor != "none" ? QColor(s.fillColor) : Qt::NoBrush);
    QPen pen(QColor(s.strokeColor), s.strokeWidth);
    if (s.lineStyle == "dash")
        pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->drawPolygon(QPolygonF(verts));
}

void ShapeGraphicsItem::drawPolylineShape(QPainter *painter)
{
    auto *pl = dynamic_cast<PolylineShape*>(m_shape);
    auto *line = dynamic_cast<LineShape*>(m_shape);
    QVector<QPointF> pts;
    bool closed = false;
    if (pl) {
        pts = pl->vertices();
        closed = pl->closed();
    } else if (line) {
        pts = line->vertices();
    }
    if (pts.isEmpty()) return;

    Style s = m_shape->style();
    QPen pen(QColor(s.strokeColor), s.strokeWidth);
    if (s.lineStyle == "dash")
        pen.setStyle(Qt::DashLine);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(pen);
    if (closed)
        painter->drawPolygon(QPolygonF(pts));
    else
        painter->drawPolyline(QPolygonF(pts));
}

void ShapeGraphicsItem::drawTextShape(QPainter *painter)
{
    auto *txt = dynamic_cast<TextShape*>(m_shape);
    if (!txt) return;
    Style s = m_shape->style();
    Transform t = m_shape->transform();

    QFont font(txt->fontFamily(), static_cast<int>(txt->fontSize()));
    painter->setFont(font);
    QColor textColor(s.strokeColor);
    if (!textColor.isValid() || s.strokeColor == QStringLiteral("none"))
        textColor = QColor(QStringLiteral("#333333"));
    painter->setPen(QPen(textColor));
    painter->setBrush(Qt::NoBrush);
    painter->drawText(QRectF(0, 0, t.width, t.height),
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, txt->text());
}

void ShapeGraphicsItem::drawStarShape(QPainter *painter)
{
    auto *star = dynamic_cast<StarShape*>(m_shape);
    if (!star) return;
    Style s = m_shape->style();
    QVector<QPointF> verts = star->vertices();
    if (verts.isEmpty()) return;

    painter->setBrush(s.fillColor != "none" ? QColor(s.fillColor) : Qt::NoBrush);
    QPen pen(QColor(s.strokeColor), s.strokeWidth);
    if (s.lineStyle == "dash")
        pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->drawPolygon(QPolygonF(verts));
}

void ShapeGraphicsItem::drawArrowShape(QPainter *painter)
{
    auto *arrow = dynamic_cast<ArrowShape*>(m_shape);
    if (!arrow) return;
    Style s = m_shape->style();
    QVector<QPointF> verts = arrow->vertices();
    if (verts.isEmpty()) return;

    painter->setBrush(s.fillColor != "none" ? QColor(s.fillColor) : Qt::NoBrush);
    QPen pen(QColor(s.strokeColor), s.strokeWidth);
    if (s.lineStyle == "dash")
        pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->drawPolygon(QPolygonF(verts));
}
