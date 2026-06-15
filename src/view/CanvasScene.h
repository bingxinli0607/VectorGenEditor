#ifndef CANVASSCENE_H
#define CANVASSCENE_H

#include <QGraphicsScene>
#include <QMap>
#include <QSet>

class Document;
class DocumentController;
class ShapeGraphicsItem;
class SelectionController;

/// View-layer — mirrors the Document model via signal-driven sync.
/// Does NOT own shape data; Document is the single source of truth.
class CanvasScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit CanvasScene(QObject *parent = nullptr);
    ~CanvasScene() override;

    void setDocument(Document *doc);
    Document *document() const { return m_document; }

    void setDocumentController(DocumentController *ctrl) { m_docCtrl = ctrl; }
    DocumentController *documentController() const { return m_docCtrl; }

    SelectionController *selectionController() const { return m_selCtrl; }

    ShapeGraphicsItem *itemForShape(const QString &shapeId) const;
    QList<ShapeGraphicsItem*> shapeItems() const { return m_itemMap.values(); }

    /// Full refresh from Document (used after loading a file)
    void syncAllFromDocument();
    void syncLayerVisibility();

    /// Expand scene rect and workspace from document shape bounds.
    void updateSceneRectFromDocument(const QSet<ShapeGraphicsItem*> *posOverrides = nullptr);
    void updateSceneExtents(const QSet<ShapeGraphicsItem*> *posOverrides = nullptr)
    { updateSceneRectFromDocument(posOverrides); }

private:
    QRectF calculateDocumentShapesBoundingRect(
        const QSet<ShapeGraphicsItem*> *posOverrides = nullptr) const;

private slots:
    void onShapeAdded(const QString &shapeId);
    void onShapeRemoved(const QString &shapeId);
    void onShapeChanged(const QString &shapeId);
    void onDocumentChanged();
    void onSceneSelectionChanged();

private:
    void connectDocument();
    void disconnectDocument();

    Document            *m_document = nullptr;
    DocumentController  *m_docCtrl  = nullptr;
    SelectionController *m_selCtrl  = nullptr;
    QMap<QString, ShapeGraphicsItem*> m_itemMap;
};

#endif // CANVASSCENE_H
