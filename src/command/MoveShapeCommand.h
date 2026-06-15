#ifndef MOVESHAPECOMMAND_H
#define MOVESHAPECOMMAND_H

#include <QUndoCommand>
#include <QPointF>
#include <QString>

class Document;

class MoveShapeCommand : public QUndoCommand
{
public:
    MoveShapeCommand(Document *doc, const QString &shapeId,
                     const QPointF &oldPos, const QPointF &newPos);

    void undo() override;
    void redo() override;
    int id() const override { return 1; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    Document *m_doc;
    QString m_shapeId;
    QPointF m_oldPos;
    QPointF m_newPos;
};

#endif // MOVESHAPECOMMAND_H
