#ifndef MODIFYSHAPECOMMAND_H
#define MODIFYSHAPECOMMAND_H

#include <QUndoCommand>
#include <QString>
#include "model/Shape.h"

class Document;

class ModifyShapeCommand : public QUndoCommand
{
public:
    ModifyShapeCommand(Document *doc, const QString &shapeId,
                       const Transform &oldTransform, const Transform &newTransform,
                       const Style &oldStyle, const Style &newStyle);

    void undo() override;
    void redo() override;

private:
    void apply(const Transform &t, const Style &s, const Transform &scaleFrom);

    Document   *m_doc;
    QString     m_shapeId;
    Transform   m_oldTransform;
    Transform   m_newTransform;
    Style       m_oldStyle;
    Style       m_newStyle;
};

#endif // MODIFYSHAPECOMMAND_H
