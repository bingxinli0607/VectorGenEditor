#ifndef DOCUMENTCONTROLLER_H
#define DOCUMENTCONTROLLER_H

#include <QObject>
#include <QUndoStack>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

#include "command/ChangeTextCommand.h"
#include "command/MoveShapesCommand.h"

class Document;
class CanvasScene;

/// Orchestrates Model-View interaction.
/// All shape creation/update goes through here; all mutations use QUndoStack.
class DocumentController : public QObject
{
    Q_OBJECT

public:
    explicit DocumentController(Document *doc, CanvasScene *scene,
                                 QObject *parent = nullptr);
    ~DocumentController() override;

    Document *document() const { return m_document; }
    CanvasScene *scene() const { return m_scene; }
    QUndoStack *undoStack() { return &m_undoStack; }

    // --- Shape creation ---
    QString createRect(double x, double y, double w, double h);
    QString createEllipse(double x, double y, double w, double h);
    QString createTriangle(double cx, double cy, double size);
    QString createRegularPolygon(int sides, double cx, double cy, double radius);
    QString createPolyline(const QVector<QPointF> &points);
    QString createLine(const QPointF &start, const QPointF &end);
    QString createText(double x, double y, const QString &text);
    QString createStar(double cx, double cy, double outerR, int points, double innerRatio);
    QString createArrow(const QPointF &start, const QPointF &end);

    // --- Shape updates (push onto QUndoStack) ---
    void updateShapePosition(const QString &id, double oldX, double oldY,
                             double newX, double newY);
    void updateShapesPosition(const QVector<ShapeMoveEntry> &entries);
    void updateShapeGeometry(const QString &id,
                              double oldX, double oldY, double oldW, double oldH, double oldRot,
                              double newX, double newY, double newW, double newH, double newRot,
                              double oldFontSize = -1.0, double newFontSize = -1.0);
    void updateShapeRotation(const QString &id, double oldRotation, double newRotation);
    void updateShapeStyle(const QString &id, const QString &fill,
                           const QString &stroke, double strokeWidth);
    void updateShapeVisible(const QString &id, bool visible);
    void updateShapeText(const QString &id, const TextProps &oldProps, const TextProps &newProps);

    // --- Deletion ---
    void deleteShape(const QString &id);

    // --- Copy/Paste ---
    void copyShape(const QString &id);
    void copyShapes(const QStringList &ids);
    void pasteShape();
    QStringList pasteShapes(double offsetX = 20.0, double offsetY = 20.0);

private:
    Document    *m_document = nullptr;
    CanvasScene *m_scene    = nullptr;
    QUndoStack   m_undoStack;
    QJsonArray   m_clipboard;
};

#endif // DOCUMENTCONTROLLER_H
