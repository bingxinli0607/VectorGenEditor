#include "RemoveShapeCommand.h"
#include "model/Document.h"
#include "model/Shape.h"
#include "model/ShapeFactory.h"

RemoveShapeCommand::RemoveShapeCommand(Document *doc, const QString &shapeId)
    : m_doc(doc), m_shapeId(shapeId)
{
    Shape *shape = m_doc->findShape(shapeId);
    if (shape) {
        m_shapeData = shape->toJson();
        setText(QString("Remove %1").arg(shape->shapeType()));
    }
}

void RemoveShapeCommand::undo()
{
    QString type = m_shapeData["type"].toString();
    auto shape = ShapeFactory::create(type);
    if (shape) {
        shape->fromJson(m_shapeData);
        m_doc->addShape(std::move(shape));
    }
}

void RemoveShapeCommand::redo()
{
    m_doc->takeShape(m_shapeId);
}
