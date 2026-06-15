#include "RotateShapeCommand.h"
#include "model/Document.h"
#include "model/Shape.h"

RotateShapeCommand::RotateShapeCommand(Document *doc, const QString &shapeId,
                                         double oldRotation, double newRotation)
    : m_doc(doc), m_shapeId(shapeId),
      m_oldRotation(oldRotation), m_newRotation(newRotation)
{
    setText("Rotate");
}

void RotateShapeCommand::undo()
{
    Shape *shape = m_doc->findShape(m_shapeId);
    if (!shape) return;
    Transform t = shape->transform();
    t.rotation = m_oldRotation;
    shape->setTransform(t);
    emit m_doc->shapeChanged(m_shapeId);
}

void RotateShapeCommand::redo()
{
    Shape *shape = m_doc->findShape(m_shapeId);
    if (!shape) return;
    Transform t = shape->transform();
    t.rotation = m_newRotation;
    shape->setTransform(t);
    emit m_doc->shapeChanged(m_shapeId);
}

bool RotateShapeCommand::mergeWith(const QUndoCommand *other)
{
    auto *cmd = dynamic_cast<const RotateShapeCommand*>(other);
    if (!cmd || cmd->m_shapeId != m_shapeId) return false;
    m_newRotation = cmd->m_newRotation;
    return true;
}
