#include "ChangeTextCommand.h"
#include "model/Document.h"
#include "model/TextShape.h"

ChangeTextCommand::ChangeTextCommand(Document *doc, const QString &shapeId,
                                     const TextProps &oldProps, const TextProps &newProps)
    : QUndoCommand(QStringLiteral("编辑文本"))
    , m_doc(doc)
    , m_shapeId(shapeId)
    , m_old(oldProps)
    , m_new(newProps)
{
}

void ChangeTextCommand::apply(const TextProps &p)
{
    if (!m_doc) return;
    Shape *shape = m_doc->findShape(m_shapeId);
    auto *txt = dynamic_cast<TextShape*>(shape);
    if (!txt) return;

    txt->setText(p.text);
    txt->setFontFamily(p.fontFamily);
    txt->setFontSize(p.fontSize);
    Style s = txt->style();
    s.strokeColor = p.textColor;
    txt->setStyle(s);
    txt->syncBoundsToText();
    emit m_doc->shapeChanged(m_shapeId);
}

void ChangeTextCommand::undo()
{
    apply(m_old);
}

void ChangeTextCommand::redo()
{
    apply(m_new);
}
