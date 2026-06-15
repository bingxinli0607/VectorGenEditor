#include "ChangeStyleCommand.h"
#include "model/Document.h"
#include "model/Shape.h"

ChangeStyleCommand::ChangeStyleCommand(Document *doc, const QString &shapeId,
                                         const Style &oldStyle, const Style &newStyle)
    : m_doc(doc), m_shapeId(shapeId),
      m_oldStyle(oldStyle), m_newStyle(newStyle)
{
    setText("Change Style");
}

void ChangeStyleCommand::undo()
{
    Shape *shape = m_doc->findShape(m_shapeId);
    if (!shape) return;
    shape->setStyle(m_oldStyle);
    emit m_doc->shapeChanged(m_shapeId);
}

void ChangeStyleCommand::redo()
{
    Shape *shape = m_doc->findShape(m_shapeId);
    if (!shape) return;
    shape->setStyle(m_newStyle);
    emit m_doc->shapeChanged(m_shapeId);
}
