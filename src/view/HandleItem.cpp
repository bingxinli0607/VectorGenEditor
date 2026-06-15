#include "HandleItem.h"
#include "view/CanvasScene.h"
#include "view/ShapeGraphicsItem.h"
#include "controller/SelectionController.h"
#include "controller/DocumentController.h"
#include "model/Document.h"
#include "model/Geometry.h"
#include "model/Shape.h"
#include "model/TextShape.h"
#include "model/Workspace.h"

#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QtMath>

namespace {

const double kMinSize = 20.0;
const double kMinFontSize = 8.0;
const double kHandleVisual = 10.0;
const double kHandleHitPad = 8.0;

ShapeGraphicsItem *resolveTarget(const HandleItem *self)
{
    if (self->targetShape())
        return self->targetShape();
    return dynamic_cast<ShapeGraphicsItem*>(self->parentItem());
}

} // namespace

QString HandleItem::roleName(HandleRole role)
{
    switch (role) {
    case HandleRole::ResizeTL: return QStringLiteral("ResizeTopLeft");
    case HandleRole::ResizeTR: return QStringLiteral("ResizeTopRight");
    case HandleRole::ResizeBL: return QStringLiteral("ResizeBottomLeft");
    case HandleRole::ResizeBR: return QStringLiteral("ResizeBottomRight");
    case HandleRole::ResizeT:  return QStringLiteral("ResizeTop");
    case HandleRole::ResizeB:  return QStringLiteral("ResizeBottom");
    case HandleRole::ResizeL:  return QStringLiteral("ResizeLeft");
    case HandleRole::ResizeR:  return QStringLiteral("ResizeRight");
    case HandleRole::Rotate:   return QStringLiteral("Rotate");
    default:                   return QStringLiteral("Move");
    }
}

HandleItem::HandleItem(HandleRole role, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_role(role)
{
    QRectF r = handleRect(QPointF(0, 0));
    setRect(r);
    setPen(QPen(QColor(255, 255, 255), 1.0));
    setBrush(colorForRole(role));
    setZValue(10000);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    switch (role) {
    case HandleRole::ResizeTL:
    case HandleRole::ResizeBR: setCursor(QCursor(Qt::SizeFDiagCursor)); break;
    case HandleRole::ResizeTR:
    case HandleRole::ResizeBL: setCursor(QCursor(Qt::SizeBDiagCursor)); break;
    case HandleRole::ResizeT:
    case HandleRole::ResizeB:  setCursor(QCursor(Qt::SizeVerCursor)); break;
    case HandleRole::ResizeL:
    case HandleRole::ResizeR:  setCursor(QCursor(Qt::SizeHorCursor)); break;
    case HandleRole::Rotate:   setCursor(QCursor(Qt::CrossCursor)); break;
    default: break;
    }
}

QPainterPath HandleItem::shape() const
{
    QPainterPath path;
    path.addRect(rect().adjusted(-kHandleHitPad, -kHandleHitPad,
                                 kHandleHitPad, kHandleHitPad));
    return path;
}

QPointF HandleItem::anchorForRole(HandleRole role, const QRectF &bounds)
{
    switch (role) {
    case HandleRole::ResizeTL: return bounds.topLeft();
    case HandleRole::ResizeTR: return bounds.topRight();
    case HandleRole::ResizeBL: return bounds.bottomLeft();
    case HandleRole::ResizeBR: return bounds.bottomRight();
    case HandleRole::ResizeT:  return QPointF(bounds.center().x(), bounds.top());
    case HandleRole::ResizeB:  return QPointF(bounds.center().x(), bounds.bottom());
    case HandleRole::ResizeL:  return QPointF(bounds.left(), bounds.center().y());
    case HandleRole::ResizeR:  return QPointF(bounds.right(), bounds.center().y());
    case HandleRole::Rotate:   return QPointF(bounds.center().x(), bounds.top() - 25);
    default:                   return bounds.topLeft();
    }
}

void HandleItem::updatePosition(const QRectF &bounds)
{
    ShapeGraphicsItem *target = resolveTarget(this);
    if (!target) return;

    const QPointF anchor = anchorForRole(m_role, bounds);
    const QPointF scenePt = target->mapToScene(anchor);
    const QRectF hr = handleRect(QPointF(0, 0));
    setPos(scenePt.x() - hr.width() / 2.0, scenePt.y() - hr.height() / 2.0);
}

QRectF HandleItem::handleRect(const QPointF &center)
{
    return QRectF(center.x() - kHandleVisual / 2.0, center.y() - kHandleVisual / 2.0,
                  kHandleVisual, kHandleVisual);
}

QColor HandleItem::colorForRole(HandleRole role)
{
    switch (role) {
    case HandleRole::Rotate:  return QColor(0, 180, 0);
    case HandleRole::Move:    return QColor(100, 100, 255);
    default:                  return QColor(70, 130, 220);
    }
}

Transform HandleItem::computeTransform(const QPointF &localPos, bool uniform) const
{
    Transform t = m_startTransform;
    const double ox = m_startTransform.x;
    const double oy = m_startTransform.y;
    const double ow = m_startTransform.width;
    const double oh = m_startTransform.height;

    switch (m_role) {
    case HandleRole::ResizeBR:
        t.width  = qMax(localPos.x(), kMinSize);
        t.height = qMax(localPos.y(), kMinSize);
        break;
    case HandleRole::ResizeTL:
        t.x = ox + localPos.x();
        t.y = oy + localPos.y();
        t.width  = qMax(ow - localPos.x(), kMinSize);
        t.height = qMax(oh - localPos.y(), kMinSize);
        break;
    case HandleRole::ResizeTR:
        t.y = oy + localPos.y();
        t.width  = qMax(localPos.x(), kMinSize);
        t.height = qMax(oh - localPos.y(), kMinSize);
        break;
    case HandleRole::ResizeBL:
        t.x = ox + localPos.x();
        t.width  = qMax(ow - localPos.x(), kMinSize);
        t.height = qMax(localPos.y(), kMinSize);
        break;
    case HandleRole::ResizeT:
        t.y = oy + localPos.y();
        t.height = qMax(oh - localPos.y(), kMinSize);
        break;
    case HandleRole::ResizeB:
        t.height = qMax(localPos.y(), kMinSize);
        break;
    case HandleRole::ResizeL:
        t.x = ox + localPos.x();
        t.width = qMax(ow - localPos.x(), kMinSize);
        break;
    case HandleRole::ResizeR:
        t.width = qMax(localPos.x(), kMinSize);
        break;
    case HandleRole::Rotate: {
        const QPointF center(ow / 2.0, oh / 2.0);
        double angle = qRadiansToDegrees(qAtan2(localPos.y() - center.y(),
                                                localPos.x() - center.x())) + 90.0;
        while (angle < 0.0) angle += 360.0;
        while (angle >= 360.0) angle -= 360.0;
        t.rotation = angle;
        break;
    }
    default:
        break;
    }

    if (uniform && m_role != HandleRole::Rotate && ow > 0.0 && oh > 0.0) {
        const double sx = t.width / ow;
        const double sy = t.height / oh;
        const double s = qMax(sx, sy);
        t.width  = qMax(kMinSize, ow * s);
        t.height = qMax(kMinSize, oh * s);
    }

    return t;
}

bool HandleItem::isTextResizeRole(HandleRole role)
{
    return role == HandleRole::ResizeTL || role == HandleRole::ResizeTR
        || role == HandleRole::ResizeBL || role == HandleRole::ResizeBR
        || role == HandleRole::ResizeT || role == HandleRole::ResizeB
        || role == HandleRole::ResizeL || role == HandleRole::ResizeR;
}

double HandleItem::scaledTextFontSize(const Transform &clamped) const
{
    if (m_startTransform.width <= 0.0 || m_startTransform.height <= 0.0)
        return m_startFontSize;

    const double wr = clamped.width / m_startTransform.width;
    const double hr = clamped.height / m_startTransform.height;
    double scale = 1.0;

    switch (m_role) {
    case HandleRole::ResizeL:
    case HandleRole::ResizeR:
        scale = wr;
        break;
    case HandleRole::ResizeT:
    case HandleRole::ResizeB:
        scale = hr;
        break;
    case HandleRole::ResizeTL:
    case HandleRole::ResizeTR:
    case HandleRole::ResizeBL:
    case HandleRole::ResizeBR:
        scale = qMin(wr, hr);
        break;
    default:
        return m_startFontSize;
    }

    return qMax(kMinFontSize, m_startFontSize * scale);
}

void HandleItem::applyTextFontScale(TextShape *txt, const Transform &clamped) const
{
    if (!txt || !isTextResizeRole(m_role)) return;
    txt->setFontSize(scaledTextFontSize(clamped));
}

void HandleItem::applyPreview(const Transform &t)
{
    ShapeGraphicsItem *target = resolveTarget(this);
    if (!target || !target->modelShape()) return;

    Shape *shape = target->modelShape();
    const QRectF workspace = target->documentController() && target->documentController()->document()
        ? target->documentController()->document()->workspaceRect()
        : Workspace::rect();
    const Transform clamped = Workspace::clampTransform(*shape, t, workspace);

    const QRectF oldB = Geometry::localBounds(m_startTransform);
    const QRectF newB = Geometry::localBounds(clamped);
    const QVector<QPointF> scaled = Geometry::scaleLocalPoints(m_startVertices, oldB, newB);

    shape->setTransform(clamped);
    if (auto *txt = dynamic_cast<TextShape*>(shape)) {
        applyTextFontScale(txt, clamped);
    } else if (!m_startVertices.isEmpty()) {
        shape->replaceLocalVertices(scaled);
    }

    target->updateFromModel();
    if (m_selCtrl) {
        m_selCtrl->updateHandles(target);
        if (target->documentController() && target->documentController()->scene())
            target->documentController()->scene()->updateSceneExtents();
    }
}

void HandleItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
    m_dragging = true;
    if (m_selCtrl)
        m_selCtrl->setHandleDragging(true);

    ShapeGraphicsItem *target = resolveTarget(this);
    if (target && target->modelShape()) {
        m_startTransform = target->modelShape()->transform();
        m_startVertices  = target->modelShape()->vertices();
        if (auto *txt = dynamic_cast<TextShape*>(target->modelShape()))
            m_startFontSize = txt->fontSize();
        if (m_selCtrl) {
            m_selCtrl->notifyStatus(
                QStringLiteral("正在缩放：shapeId=%1, role=%2")
                    .arg(target->modelShape()->id(), roleName(m_role)));
        }
    }

    grabMouse();
}

void HandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_dragging) return;
    event->accept();

    ShapeGraphicsItem *target = resolveTarget(this);
    if (!target) return;

    const QPointF local = target->mapFromScene(event->scenePos());
    const bool uniform = event->modifiers() & Qt::ShiftModifier;
    const Transform t = computeTransform(local, uniform);
    applyPreview(t);

    if (m_selCtrl && target->modelShape()) {
        if (m_role == HandleRole::Rotate) {
            m_selCtrl->notifyStatus(
                QStringLiteral("正在旋转：angle=%1°").arg(t.rotation, 0, 'f', 1));
        } else {
            m_selCtrl->notifyStatus(
                QStringLiteral("正在缩放：W=%1 H=%2")
                    .arg(t.width, 0, 'f', 1)
                    .arg(t.height, 0, 'f', 1));
        }
    }
}

void HandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_dragging) return;
    m_dragging = false;
    ungrabMouse();
    if (m_selCtrl)
        m_selCtrl->setHandleDragging(false);
    event->accept();

    ShapeGraphicsItem *target = resolveTarget(this);
    if (!target || !target->modelShape() || !m_selCtrl) return;

    const QPointF local = target->mapFromScene(event->scenePos());
    const bool uniform = event->modifiers() & Qt::ShiftModifier;
    Transform newT = computeTransform(local, uniform);

    const QRectF workspace = target->documentController() && target->documentController()->document()
        ? target->documentController()->document()->workspaceRect()
        : Workspace::rect();
    newT = Workspace::clampTransform(*target->modelShape(), newT, workspace);

    double oldFontSize = -1.0;
    double newFontSize = -1.0;
    if (auto *txt = dynamic_cast<TextShape*>(target->modelShape())) {
        if (isTextResizeRole(m_role)) {
            oldFontSize = m_startFontSize;
            newFontSize = scaledTextFontSize(newT);
            Q_UNUSED(txt);
        }
    }

    Shape *shape = target->modelShape();
    shape->setTransform(m_startTransform);
    if (!m_startVertices.isEmpty())
        shape->replaceLocalVertices(m_startVertices);
    target->updateFromModel();

    if (m_role == HandleRole::Rotate) {
        if (!qFuzzyCompare(m_startTransform.rotation, newT.rotation)) {
            m_selCtrl->onItemRotated(target, m_startTransform.rotation, newT.rotation);
            m_selCtrl->notifyStatus(
                QStringLiteral("旋转完成：oldRotation=%1°, newRotation=%2°")
                    .arg(m_startTransform.rotation, 0, 'f', 1)
                    .arg(newT.rotation, 0, 'f', 1));
        }
    } else {
        const QRectF oldR(m_startTransform.x, m_startTransform.y,
                          m_startTransform.width, m_startTransform.height);
        const QRectF newR(newT.x, newT.y, newT.width, newT.height);
        const bool changed = !qFuzzyCompare(oldR.x(), newR.x())
            || !qFuzzyCompare(oldR.y(), newR.y())
            || !qFuzzyCompare(oldR.width(), newR.width())
            || !qFuzzyCompare(oldR.height(), newR.height())
            || !qFuzzyCompare(m_startTransform.rotation, newT.rotation);
        if (changed) {
            m_selCtrl->onItemResized(target, oldR, newR,
                                     m_startTransform.rotation, newT.rotation,
                                     oldFontSize, newFontSize);
            m_selCtrl->notifyStatus(
                QStringLiteral("缩放完成：oldRect=(%1,%2 %3×%4) newRect=(%5,%6 %7×%8)")
                    .arg(oldR.x(), 0, 'f', 0).arg(oldR.y(), 0, 'f', 0)
                    .arg(oldR.width(), 0, 'f', 0).arg(oldR.height(), 0, 'f', 0)
                    .arg(newR.x(), 0, 'f', 0).arg(newR.y(), 0, 'f', 0)
                    .arg(newR.width(), 0, 'f', 0).arg(newR.height(), 0, 'f', 0));
        }
    }
}
