#ifndef MOVESHAPESCOMMAND_H
#define MOVESHAPESCOMMAND_H

#include <QUndoCommand>
#include <QPointF>
#include <QString>
#include <QVector>

class Document;

struct ShapeMoveEntry {
    QString shapeId;
    QPointF oldPos;
    QPointF newPos;
};

class MoveShapesCommand : public QUndoCommand
{
public:
    MoveShapesCommand(Document *doc, const QVector<ShapeMoveEntry> &entries);

    void undo() override;
    void redo() override;

private:
    void applyPositions(const QVector<ShapeMoveEntry> &entries, bool useNew);

    Document *m_doc = nullptr;
    QVector<ShapeMoveEntry> m_entries;
};

#endif // MOVESHAPESCOMMAND_H
