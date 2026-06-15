#ifndef RESIZESHAPECOMMAND_H
#define RESIZESHAPECOMMAND_H

#include <QUndoCommand>
#include <QString>
#include "model/Shape.h"  // for Transform

class Document;

class ResizeShapeCommand : public QUndoCommand
{
public:
    ResizeShapeCommand(Document *doc, const QString &shapeId,
                       const Transform &oldTransform,
                       const Transform &newTransform,
                       double oldFontSize = -1.0,
                       double newFontSize = -1.0);

    void undo() override;
    void redo() override;
    int id() const override { return 2; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    void applyTransform(const Transform &t, double fontSize);

    Document *m_doc;
    QString m_shapeId;
    Transform m_oldTransform;
    Transform m_newTransform;
    double m_oldFontSize = -1.0;
    double m_newFontSize = -1.0;
};

#endif // RESIZESHAPECOMMAND_H
