#include "ModifyShapeCommand.h"
#include "model/Document.h"
#include "model/Shape.h"
#include "model/Geometry.h"

ModifyShapeCommand::ModifyShapeCommand(Document *doc, const QString &shapeId,
                                         const Transform &oldTransform, const Transform &newTransform,
                                         const Style &oldStyle, const Style &newStyle)
    : m_doc(doc), m_shapeId(shapeId),
      m_oldTransform(oldTransform), m_newTransform(newTransform),
      m_oldStyle(oldStyle), m_newStyle(newStyle)
{
    setText("Modify Shape");
}

void ModifyShapeCommand::apply(const Transform &t, const Style &s, const Transform &scaleFrom)
{
    Shape *shape = m_doc->findShape(m_shapeId);
    if (!shape) return;
    const QRectF oldB = Geometry::localBounds(scaleFrom);
    shape->setTransform(t);
    shape->scaleLocalGeometry(oldB, Geometry::localBounds(t));
    shape->setStyle(s);
    emit m_doc->shapeChanged(m_shapeId);
}

void ModifyShapeCommand::undo()
{
    apply(m_oldTransform, m_oldStyle, m_newTransform);
}

void ModifyShapeCommand::redo()
{
    apply(m_newTransform, m_newStyle, m_oldTransform);
}
