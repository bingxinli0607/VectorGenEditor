#include "ResizeShapeCommand.h"
#include "model/Document.h"
#include "model/Shape.h"
#include "model/TextShape.h"
#include "model/Geometry.h"

ResizeShapeCommand::ResizeShapeCommand(Document *doc, const QString &shapeId,
                                         const Transform &oldTransform,
                                         const Transform &newTransform,
                                         double oldFontSize,
                                         double newFontSize)
    : m_doc(doc), m_shapeId(shapeId),
      m_oldTransform(oldTransform), m_newTransform(newTransform),
      m_oldFontSize(oldFontSize), m_newFontSize(newFontSize)
{
    setText(QStringLiteral("缩放"));
}

void ResizeShapeCommand::applyTransform(const Transform &t, double fontSize)
{
    Shape *shape = m_doc->findShape(m_shapeId);
    if (!shape) return;

    const QRectF currentB = Geometry::localBounds(shape->transform());
    shape->setTransform(t);

    if (fontSize >= 0.0) {
        if (auto *txt = dynamic_cast<TextShape*>(shape)) {
            txt->setFontSize(fontSize);
        }
    } else {
        shape->scaleLocalGeometry(currentB, Geometry::localBounds(t));
    }

    emit m_doc->shapeChanged(m_shapeId);
}

void ResizeShapeCommand::undo()
{
    applyTransform(m_oldTransform, m_oldFontSize);
}

void ResizeShapeCommand::redo()
{
    applyTransform(m_newTransform, m_newFontSize);
}

bool ResizeShapeCommand::mergeWith(const QUndoCommand *other)
{
    auto *cmd = dynamic_cast<const ResizeShapeCommand*>(other);
    if (!cmd || cmd->m_shapeId != m_shapeId) return false;
    m_newTransform = cmd->m_newTransform;
    if (cmd->m_newFontSize >= 0.0)
        m_newFontSize = cmd->m_newFontSize;
    return true;
}
