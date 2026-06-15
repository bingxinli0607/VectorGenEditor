#include "MoveShapeCommand.h"
#include "model/Document.h"
#include "model/Shape.h"

MoveShapeCommand::MoveShapeCommand(Document *doc, const QString &shapeId,
                                     const QPointF &oldPos, const QPointF &newPos)
    : m_doc(doc), m_shapeId(shapeId), m_oldPos(oldPos), m_newPos(newPos)
{
    setText("Move");
}

void MoveShapeCommand::undo()
{
    Shape *shape = m_doc->findShape(m_shapeId);
    if (!shape) return;
    Transform t = shape->transform();
    shape->translateBy(m_oldPos.x() - t.x, m_oldPos.y() - t.y);
    emit m_doc->shapeChanged(m_shapeId);
}

void MoveShapeCommand::redo()
{
    Shape *shape = m_doc->findShape(m_shapeId);
    if (!shape) return;
    Transform t = shape->transform();
    shape->translateBy(m_newPos.x() - t.x, m_newPos.y() - t.y);
    emit m_doc->shapeChanged(m_shapeId);
}

bool MoveShapeCommand::mergeWith(const QUndoCommand *other)
{
    auto *cmd = dynamic_cast<const MoveShapeCommand*>(other);
    if (!cmd || cmd->m_shapeId != m_shapeId) return false;
    m_newPos = cmd->m_newPos;
    return true;
}
