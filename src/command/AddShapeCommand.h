#ifndef ADDSHAPECOMMAND_H
#define ADDSHAPECOMMAND_H

#include <QUndoCommand>
#include <memory>

class Document;
class Shape;

class AddShapeCommand : public QUndoCommand
{
public:
    AddShapeCommand(Document *doc, std::unique_ptr<Shape> shape);

    void undo() override;
    void redo() override;

private:
    Document *m_doc;
    std::unique_ptr<Shape> m_shape;
    QString m_shapeId;
};

#endif // ADDSHAPECOMMAND_H
