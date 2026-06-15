#include "MoveShapesCommand.h"
#include "model/Document.h"
#include "model/Shape.h"

MoveShapesCommand::MoveShapesCommand(Document *doc, const QVector<ShapeMoveEntry> &entries)
    : m_doc(doc), m_entries(entries)
{
    setText(QStringLiteral("Move shapes"));
}

void MoveShapesCommand::applyPositions(const QVector<ShapeMoveEntry> &entries, bool useNew)
{
    for (const auto &e : entries) {
        Shape *shape = m_doc->findShape(e.shapeId);
        if (!shape) continue;
        const QPointF target = useNew ? e.newPos : e.oldPos;
        Transform t = shape->transform();
        shape->translateBy(target.x() - t.x, target.y() - t.y);
        emit m_doc->shapeChanged(e.shapeId);
    }
}

void MoveShapesCommand::undo()
{
    applyPositions(m_entries, false);
}

void MoveShapesCommand::redo()
{
    applyPositions(m_entries, true);
}
