#ifndef CANVASVIEW_H
#define CANVASVIEW_H

#include <QGraphicsView>
#include <QPointF>

class QRubberBand;
class QGraphicsLineItem;
class DocumentController;
class SelectionController;

enum class CanvasTool {
    Select,
    Line,
    Text,
};

class CanvasView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CanvasView(QGraphicsScene *scene, QWidget *parent = nullptr);
    ~CanvasView() override;

    void setDocumentController(DocumentController *ctrl) { m_docCtrl = ctrl; }
    void setSelectionController(SelectionController *ctrl) { m_selCtrl = ctrl; }

    void setActiveTool(CanvasTool tool);
    CanvasTool activeTool() const { return m_activeTool; }

    void zoomIn();
    void zoomOut();
    void fitAll();
    /// Fit workspace with a readable default zoom (60%–100%).
    void fitWorkspaceDefault();

    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }

    double zoomLevel() const { return m_zoomLevel; }

    void cancelActiveOperation();

signals:
    void mouseScenePosChanged(const QPointF &pos);
    void zoomLevelChanged(double level);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void notifyZoomChanged();
    /// True when view pos hits a shape or its resize/rotate handle (must not start box-select).
    bool hitInteractiveAt(const QPoint &viewPos) const;
    void beginRubberBand(const QPoint &pos);
    void updateRubberBand(const QPoint &pos);
    void finishRubberBand(const QPoint &pos, Qt::KeyboardModifiers mods);
    void beginLineDrag(const QPointF &scenePos);
    void updateLinePreview(const QPointF &scenePos);
    void finishLineDrag(const QPointF &scenePos);
    void placeTextAt(const QPointF &scenePos);

    double m_zoomLevel = 1.0;
    bool m_gridVisible = true;
    double m_gridSize = 20.0;

    CanvasTool m_activeTool = CanvasTool::Select;
    DocumentController *m_docCtrl = nullptr;
    SelectionController *m_selCtrl = nullptr;

    QRubberBand *m_rubberBand = nullptr;
    QPoint m_rubberOrigin;
    bool m_rubberActive = false;

    bool m_lineDragging = false;
    QPointF m_lineStartScene;
    QGraphicsLineItem *m_linePreview = nullptr;
};

#endif // CANVASVIEW_H
