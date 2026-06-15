#ifndef CHANGESTYLECOMMAND_H
#define CHANGESTYLECOMMAND_H

#include <QUndoCommand>
#include <QString>
#include "model/Shape.h"  // for Style

class Document;

class ChangeStyleCommand : public QUndoCommand
{
public:
    ChangeStyleCommand(Document *doc, const QString &shapeId,
                       const Style &oldStyle, const Style &newStyle);

    void undo() override;
    void redo() override;

private:
    Document *m_doc;
    QString m_shapeId;
    Style m_oldStyle;
    Style m_newStyle;
};

#endif // CHANGESTYLECOMMAND_H
