#ifndef REMOVESHAPECOMMAND_H
#define REMOVESHAPECOMMAND_H

#include <QUndoCommand>
#include <QJsonObject>
#include <QString>

class Document;

class RemoveShapeCommand : public QUndoCommand
{
public:
    RemoveShapeCommand(Document *doc, const QString &shapeId);

    void undo() override;
    void redo() override;

private:
    Document *m_doc;
    QString m_shapeId;
    QJsonObject m_shapeData;
    QString m_layerName;  // which layer the shape was in
};

#endif // REMOVESHAPECOMMAND_H
