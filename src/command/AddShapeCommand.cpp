#include "AddShapeCommand.h"
#include "model/Document.h"
#include "model/Shape.h"

AddShapeCommand::AddShapeCommand(Document *doc, std::unique_ptr<Shape> shape)
    : m_doc(doc), m_shape(std::move(shape))
{
    m_shapeId = m_shape->id();
    setText(QString("Add %1").arg(m_shape->shapeType()));
}

void AddShapeCommand::undo()
{
    m_shape = m_doc->takeShape(m_shapeId);
}

void AddShapeCommand::redo()
{
    m_doc->addShape(std::move(m_shape));
}
