#include "CanvasView.h"
#include "model/Artboard.h"
#include "model/Workspace.h"
#include "controller/DocumentController.h"
#include "model/Document.h"
#include "controller/SelectionController.h"
#include "view/ShapeGraphicsItem.h"
#include "view/HandleItem.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QVector>
#include <QPainter>
#include <QRubberBand>
#include <QGraphicsLineItem>
#include <QtMath>

CanvasView::CanvasView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setRenderHint(QPainter::Antialiasing, true);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setFrameShape(QFrame::NoFrame);
    setStyleSheet("background: transparent;");
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, viewport());
}

CanvasView::~CanvasView()
{
    if (m_linePreview && scene()) {
        scene()->removeItem(m_linePreview);
        delete m_linePreview;
    }
}

void CanvasView::setActiveTool(CanvasTool tool)
{
    cancelActiveOperation();
    m_activeTool = tool;
}

void CanvasView::cancelActiveOperation()
{
    if (m_rubberActive) {
        m_rubberBand->hide();
        m_rubberActive = false;
    }
    if (m_lineDragging) {
        m_lineDragging = false;
        if (m_linePreview && scene()) {
            scene()->removeItem(m_linePreview);
            delete m_linePreview;
            m_linePreview = nullptr;
        }
    }
}

bool CanvasView::hitInteractiveAt(const QPoint &viewPos) const
{
    const QGraphicsItem *item = itemAt(viewPos);
    if (!item) return false;
    if (dynamic_cast<const ShapeGraphicsItem*>(item))
        return true;
    if (dynamic_cast<const HandleItem*>(item))
        return true;
    for (const QGraphicsItem *p = item->parentItem(); p; p = p->parentItem()) {
        if (dynamic_cast<const ShapeGraphicsItem*>(p))
            return true;
    }
    return false;
}

void CanvasView::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    viewport()->update();
}

void CanvasView::notifyZoomChanged()
{
    m_zoomLevel = transform().m11();
    emit zoomLevelChanged(m_zoomLevel);
}

void CanvasView::zoomIn()
{
    scale(1.15, 1.15);
    notifyZoomChanged();
}

void CanvasView::zoomOut()
{
    scale(1.0 / 1.15, 1.0 / 1.15);
    notifyZoomChanged();
}

void CanvasView::fitAll()
{
    if (scene())
        fitInView(Artboard::rect(), Qt::KeepAspectRatio);
    notifyZoomChanged();
}

void CanvasView::fitWorkspaceDefault()
{
    if (!scene()) return;

    QRectF paper = Workspace::defaultPageRect();
    if (m_docCtrl && m_docCtrl->document())
        paper = Workspace::sanitizeRect(m_docCtrl->document()->pageRect());
    if (!paper.isValid() || paper.width() < 1.0 || paper.height() < 1.0)
        return;

    fitInView(paper, Qt::KeepAspectRatio);

    const double current = transform().m11();
    const double target = qBound(0.60, current, 1.0);
    if (!qFuzzyCompare(current, target)) {
        resetTransform();
        scale(target, target);
    }
    centerOn(paper.center());
    notifyZoomChanged();
}

void CanvasView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor(232, 234, 237));

    QRectF pageRect = Workspace::defaultPageRect();
    if (m_docCtrl && m_docCtrl->document())
        pageRect = Workspace::sanitizeRect(m_docCtrl->document()->pageRect());
    if (!pageRect.isValid() || pageRect.width() < 1.0 || pageRect.height() < 1.0)
        return;

    const QRectF pageDraw = pageRect.intersected(rect);
    if (pageDraw.isEmpty())
        return;

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 18));
    painter->drawRect(pageDraw.translated(3, 4));
    painter->restore();

    painter->fillRect(pageDraw, Qt::white);

    painter->setPen(QPen(QColor(200, 208, 220), 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(pageRect);

    if (!m_gridVisible)
        return;

    painter->save();
    painter->setClipRect(pageDraw);
    painter->setPen(QPen(QColor(245, 247, 250), 0.8));

    const double left = qFloor(pageDraw.left() / m_gridSize) * m_gridSize;
    const double top  = qFloor(pageDraw.top() / m_gridSize) * m_gridSize;

    QVector<QLineF> lines;
    lines.reserve(512);
    constexpr int kMaxGridLines = 4000;
    for (double x = left; x <= pageDraw.right() && lines.size() < kMaxGridLines; x += m_gridSize)
        lines.append(QLineF(x, pageDraw.top(), x, pageDraw.bottom()));
    for (double y = top; y <= pageDraw.bottom() && lines.size() < kMaxGridLines; y += m_gridSize)
        lines.append(QLineF(pageDraw.left(), y, pageDraw.right(), y));

    if (!lines.isEmpty())
        painter->drawLines(lines.constData(), lines.size());
    painter->restore();
}

void CanvasView::beginRubberBand(const QPoint &pos)
{
    m_rubberOrigin = pos;
    m_rubberBand->setGeometry(QRect(m_rubberOrigin, QSize()));
    m_rubberBand->show();
    m_rubberActive = true;
}

void CanvasView::updateRubberBand(const QPoint &pos)
{
    if (!m_rubberActive) return;
    m_rubberBand->setGeometry(QRect(m_rubberOrigin, pos).normalized());
}

void CanvasView::finishRubberBand(const QPoint &pos, Qt::KeyboardModifiers mods)
{
    if (!m_rubberActive || !m_selCtrl) return;
    const QRect viewRect = QRect(m_rubberOrigin, pos).normalized();
    m_rubberBand->hide();
    m_rubberActive = false;

    if (viewRect.width() < 3 && viewRect.height() < 3)
        return;

    const QPointF p1 = mapToScene(viewRect.topLeft());
    const QPointF p2 = mapToScene(viewRect.bottomRight());
    const QRectF sceneRect = QRectF(p1, p2).normalized();
    const bool additive = mods & (Qt::ControlModifier | Qt::ShiftModifier);
    m_selCtrl->selectInRect(sceneRect, additive);
}

void CanvasView::beginLineDrag(const QPointF &scenePos)
{
    m_lineStartScene = scenePos;
    m_lineDragging = true;
    if (!scene()) return;
    m_linePreview = new QGraphicsLineItem(QLineF(scenePos, scenePos));
    m_linePreview->setPen(QPen(QColor(70, 130, 220), 2, Qt::DashLine));
    m_linePreview->setZValue(500);
    scene()->addItem(m_linePreview);
}

void CanvasView::updateLinePreview(const QPointF &scenePos)
{
    if (!m_lineDragging || !m_linePreview) return;
    m_linePreview->setLine(QLineF(m_lineStartScene, scenePos));
}

namespace {

bool scenePointInWorkspace(const DocumentController *docCtrl, const QPointF &p)
{
    if (docCtrl && docCtrl->document())
        return docCtrl->document()->workspaceRect().contains(p);
    return Workspace::containsPoint(p);
}

} // namespace

void CanvasView::finishLineDrag(const QPointF &scenePos)
{
    if (!m_lineDragging) return;
    m_lineDragging = false;
    if (m_linePreview && scene()) {
        scene()->removeItem(m_linePreview);
        delete m_linePreview;
        m_linePreview = nullptr;
    }

    const double dx = scenePos.x() - m_lineStartScene.x();
    const double dy = scenePos.y() - m_lineStartScene.y();
    if (qSqrt(dx * dx + dy * dy) < 4.0 || !m_docCtrl)
        return;

    if (!scenePointInWorkspace(m_docCtrl, m_lineStartScene)
        || !scenePointInWorkspace(m_docCtrl, scenePos))
        return;

    m_docCtrl->createLine(m_lineStartScene, scenePos);
}

void CanvasView::placeTextAt(const QPointF &scenePos)
{
    if (!m_docCtrl) return;
    if (!scenePointInWorkspace(m_docCtrl, scenePos))
        return;
    m_docCtrl->createText(scenePos.x(), scenePos.y(), QStringLiteral("双击编辑"));
}

void CanvasView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const bool mod = event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);

        if (m_activeTool == CanvasTool::Select && !hitInteractiveAt(event->pos())) {
            if (!mod && m_selCtrl)
                m_selCtrl->deselectAll();
            beginRubberBand(event->pos());
            event->accept();
            return;
        }

        if (m_activeTool == CanvasTool::Line && !hitInteractiveAt(event->pos())) {
            const QPointF scenePos = mapToScene(event->pos());
            if (scenePointInWorkspace(m_docCtrl, scenePos))
                beginLineDrag(scenePos);
            event->accept();
            return;
        }

        if (m_activeTool == CanvasTool::Text && !hitInteractiveAt(event->pos())) {
            placeTextAt(mapToScene(event->pos()));
            event->accept();
            return;
        }
    }

    QGraphicsView::mousePressEvent(event);
    emit mouseScenePosChanged(mapToScene(event->pos()));
}

void CanvasView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_rubberActive && m_selCtrl && m_selCtrl->isHandleDragging()) {
        m_rubberBand->hide();
        m_rubberActive = false;
    }
    if (m_rubberActive)
        updateRubberBand(event->pos());
    if (m_lineDragging)
        updateLinePreview(mapToScene(event->pos()));

    QGraphicsView::mouseMoveEvent(event);
    emit mouseScenePosChanged(mapToScene(event->pos()));
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_rubberActive && event->button() == Qt::LeftButton) {
        finishRubberBand(event->pos(), event->modifiers());
        event->accept();
        return;
    }
    if (m_lineDragging && event->button() == Qt::LeftButton) {
        finishLineDrag(mapToScene(event->pos()));
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0)
            zoomIn();
        else
            zoomOut();
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void CanvasView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        cancelActiveOperation();
        if (m_selCtrl)
            m_selCtrl->deselectAll();
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}
