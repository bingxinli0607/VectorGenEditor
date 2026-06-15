#ifndef CHANGETEXTCOMMAND_H
#define CHANGETEXTCOMMAND_H

#include <QUndoCommand>
#include <QString>

class Document;

struct TextProps {
    QString text;
    QString fontFamily;
    double fontSize = 24.0;
    QString textColor = QStringLiteral("#333333");
};

class ChangeTextCommand : public QUndoCommand
{
public:
    ChangeTextCommand(Document *doc, const QString &shapeId,
                      const TextProps &oldProps, const TextProps &newProps);

    void undo() override;
    void redo() override;

private:
    void apply(const TextProps &p);

    Document  *m_doc = nullptr;
    QString    m_shapeId;
    TextProps  m_old;
    TextProps  m_new;
};

#endif // CHANGETEXTCOMMAND_H
