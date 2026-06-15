#include "DocumentController.h"
#include "model/Document.h"
#include "model/Layer.h"
#include "model/RectShape.h"
#include "model/EllipseShape.h"
#include "model/PolygonShape.h"
#include "model/PolylineShape.h"
#include "model/TextShape.h"
#include "model/StarShape.h"
#include "model/ArrowShape.h"
#include "model/LineShape.h"
#include "command/ChangeTextCommand.h"
#include "view/CanvasScene.h"
#include "model/ShapeFactory.h"
#include "command/AddShapeCommand.h"
#include "command/RemoveShapeCommand.h"
#include "command/MoveShapeCommand.h"
#include "command/MoveShapesCommand.h"
#include "command/ResizeShapeCommand.h"
#include "command/ChangeStyleCommand.h"
#include "command/RotateShapeCommand.h"
#include "model/Workspace.h"
#include "model/Geometry.h"

DocumentController::DocumentController(Document *doc, CanvasScene *scene,
                                         QObject *parent)
    : QObject(parent), m_document(doc), m_scene(scene)
{
}

DocumentController::~DocumentController() = default;

namespace {

QRectF documentShapeBounds(const Document *doc)
{
    QRectF bounds;
    if (!doc) return bounds;
    for (const auto &layer : doc->layers()) {
        for (const auto &shape : layer->shapes())
            bounds = bounds.united(Geometry::sceneBounds(*shape));
    }
    return bounds;
}

void ensureWorkspaceFitsShape(Document *doc, const Shape &shape, double margin = 200.0)
{
    if (!doc) return;
    QRectF bounds = documentShapeBounds(doc);
    bounds = bounds.united(Geometry::sceneBounds(shape));
    doc->setPageRect(Workspace::sanitizeRect(
        Workspace::pageRectForShapeBounds(bounds, margin)));
}

} // namespace

// --- Shape creation helpers ---

QString DocumentController::createRect(double x, double y, double w, double h)
{
    auto s = std::make_unique<RectShape>();
    s->setTransform({x, y, w, h, 0.0});
    ensureWorkspaceFitsShape(m_document, *s);
    QString id = s->id();
    m_undoStack.push(new AddShapeCommand(m_document, std::move(s)));
    return id;
}

QString DocumentController::createEllipse(double x, double y, double w, double h)
{
    auto s = std::make_unique<EllipseShape>();
    s->setTransform({x, y, w, h, 0.0});
    ensureWorkspaceFitsShape(m_document, *s);
    QString id = s->id();
    m_undoStack.push(new AddShapeCommand(m_document, std::move(s)));
    return id;
}

QString DocumentController::createTriangle(double cx, double cy, double size)
{
    auto s = PolygonShape::createTriangle(cx, cy, size);
    ensureWorkspaceFitsShape(m_document, *s);
    QString id = s->id();
    m_undoStack.push(new AddShapeCommand(m_document, std::move(s)));
    return id;
}

QString DocumentController::createRegularPolygon(int sides, double cx, double cy, double radius)
{
    auto s = PolygonShape::createRegularPolygon(sides, cx, cy, radius);
    ensureWorkspaceFitsShape(m_document, *s);
    QString id = s->id();
    m_undoStack.push(new AddShapeCommand(m_document, std::move(s)));
    return id;
}

QString DocumentController::createPolyline(const QVector<QPointF> &points)
{
    auto s = std::make_unique<PolylineShape>();
    s->setPoints(points);
    ensureWorkspaceFitsShape(m_document, *s);
    QString id = s->id();
    m_undoStack.push(new AddShapeCommand(m_document, std::move(s)));
    return id;
}

QString DocumentController::createLine(const QPointF &start, const QPointF &end)
{
    auto s = std::make_unique<LineShape>();
    s->setEndpointsFromScene(start, end);
    Style st = s->style();
    st.fillColor = QStringLiteral("none");
    s->setStyle(st);
    ensureWorkspaceFitsShape(m_document, *s);
    QString id = s->id();
    m_undoStack.push(new AddShapeCommand(m_document, std::move(s)));
    return id;
}

QString DocumentController::createText(double x, double y, const QString &text)
{
    auto s = std::make_unique<TextShape>();
    s->setFontSize(26.0);
    s->setText(text);
    Style st = s->style();
    st.fillColor = QStringLiteral("none");
    st.strokeColor = QStringLiteral("#333333");
    s->setStyle(st);
    s->syncBoundsToText();

    Transform t = s->transform();
    t.x = x;
    t.y = y;
    s->setTransform(t);
    ensureWorkspaceFitsShape(m_document, *s);
    t = Workspace::clampTransform(*s, t, m_document->workspaceRect());
    s->setTransform(t);
    QString id = s->id();
    m_undoStack.push(new AddShapeCommand(m_document, std::move(s)));
    return id;
}

QString DocumentController::createStar(double cx, double cy, double outerR,
                                        int points, double innerRatio)
{
    auto s = std::make_unique<StarShape>();
    s->setParameters(points, innerRatio);
    s->setVerticesFromScene(s->generateVertices(cx, cy, outerR));
    ensureWorkspaceFitsShape(m_document, *s);
    QString id = s->id();
    m_undoStack.push(new AddShapeCommand(m_document, std::move(s)));
    return id;
}

QString DocumentController::createArrow(const QPointF &start, const QPointF &end)
{
    auto s = std::make_unique<ArrowShape>();
    s->setParameters(15, 20);
    s->setStartEnd(start, end);
    ensureWorkspaceFitsShape(m_document, *s);
    QString id = s->id();
    m_undoStack.push(new AddShapeCommand(m_document, std::move(s)));
    return id;
}

// --- Shape updates (push onto QUndoStack) ---

void DocumentController::updateShapePosition(const QString &id,
                                               double oldX, double oldY,
                                               double newX, double newY)
{
    m_undoStack.push(new MoveShapeCommand(m_document, id,
                                            QPointF(oldX, oldY),
                                            QPointF(newX, newY)));
}

void DocumentController::updateShapesPosition(const QVector<ShapeMoveEntry> &entries)
{
    if (entries.isEmpty()) return;
    if (entries.size() == 1) {
        const auto &e = entries.first();
        updateShapePosition(e.shapeId,
                            e.oldPos.x(), e.oldPos.y(),
                            e.newPos.x(), e.newPos.y());
        return;
    }
    m_undoStack.push(new MoveShapesCommand(m_document, entries));
}

void DocumentController::updateShapeGeometry(const QString &id,
                                               double oldX, double oldY, double oldW, double oldH, double oldRot,
                                               double newX, double newY, double newW, double newH, double newRot,
                                               double oldFontSize, double newFontSize)
{
    Transform oldT = {oldX, oldY, oldW, oldH, oldRot};
    Transform newT = {newX, newY, newW, newH, newRot};
    Shape *shape = m_document->findShape(id);
    if (shape) {
        ensureWorkspaceFitsShape(m_document, *shape);
        newT = Workspace::clampTransform(*shape, newT, m_document->workspaceRect());
    }
    m_undoStack.push(new ResizeShapeCommand(m_document, id, oldT, newT,
                                            oldFontSize, newFontSize));
}

void DocumentController::updateShapeRotation(const QString &id,
                                               double oldRotation, double newRotation)
{
    m_undoStack.push(new RotateShapeCommand(m_document, id, oldRotation, newRotation));
}

void DocumentController::updateShapeStyle(const QString &id, const QString &fill,
                                            const QString &stroke, double strokeWidth)
{
    Shape *shape = m_document->findShape(id);
    if (!shape) return;
    Style oldS = shape->style();
    Style newS = oldS;
    newS.fillColor   = fill;
    newS.strokeColor = stroke;
    newS.strokeWidth = strokeWidth;
    m_undoStack.push(new ChangeStyleCommand(m_document, id, oldS, newS));
}

void DocumentController::updateShapeVisible(const QString &id, bool visible)
{
    Shape *shape = m_document->findShape(id);
    if (!shape) return;
    Style oldS = shape->style();
    Style newS = oldS;
    newS.visible = visible;
    m_undoStack.push(new ChangeStyleCommand(m_document, id, oldS, newS));
}

void DocumentController::updateShapeText(const QString &id,
                                         const TextProps &oldProps,
                                         const TextProps &newProps)
{
    m_undoStack.push(new ChangeTextCommand(m_document, id, oldProps, newProps));
}

void DocumentController::deleteShape(const QString &id)
{
    m_undoStack.push(new RemoveShapeCommand(m_document, id));
}

// --- Copy/Paste ---

void DocumentController::copyShape(const QString &id)
{
    copyShapes({id});
}

void DocumentController::copyShapes(const QStringList &ids)
{
    m_clipboard = QJsonArray();
    for (const QString &id : ids) {
        Shape *shape = m_document->findShape(id);
        if (shape)
            m_clipboard.append(shape->toJson());
    }
}

void DocumentController::pasteShape()
{
    pasteShapes();
}

QStringList DocumentController::pasteShapes(double offsetX, double offsetY)
{
    if (m_clipboard.isEmpty()) return {};

    QStringList newIds;
    m_undoStack.beginMacro(QStringLiteral("粘贴"));

    for (const QJsonValue &val : m_clipboard) {
        const QJsonObject obj = val.toObject();
        if (obj.isEmpty()) continue;

        const QString type = obj["type"].toString();
        auto shape = ShapeFactory::create(type);
        if (!shape) continue;

        shape->fromJson(obj);
        shape->setId(generateShapeId());
        shape->translateBy(offsetX, offsetY);
        ensureWorkspaceFitsShape(m_document, *shape);

        const QString id = shape->id();
        newIds.append(id);
        m_undoStack.push(new AddShapeCommand(m_document, std::move(shape)));
    }

    m_undoStack.endMacro();
    return newIds;
}
