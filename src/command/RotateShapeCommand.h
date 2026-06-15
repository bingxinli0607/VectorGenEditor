#ifndef ROTATESHAPECOMMAND_H
#define ROTATESHAPECOMMAND_H

#include <QUndoCommand>
#include <QString>
#include "model/Shape.h"

class Document;

class RotateShapeCommand : public QUndoCommand
{
public:
    RotateShapeCommand(Document *doc, const QString &shapeId,
                       double oldRotation, double newRotation);

    void undo() override;
    void redo() override;
    bool mergeWith(const QUndoCommand *other) override;

private:
    Document   *m_doc;
    QString     m_shapeId;
    double      m_oldRotation;
    double      m_newRotation;
};

#endif // ROTATESHAPECOMMAND_H
