#include "TextShape.h"
#include <QFontMetrics>

std::unique_ptr<Shape> TextShape::clone() const
{
    auto s = std::make_unique<TextShape>();
    s->setId(id());
    s->setTransform(transform());
    s->setStyle(style());
    s->m_text       = m_text;
    s->m_fontFamily = m_fontFamily;
    s->m_fontSize   = m_fontSize;
    return s;
}

void TextShape::setText(const QString &t)       { m_text = t; }
void TextShape::setFontFamily(const QString &f) { m_fontFamily = f; }
void TextShape::setFontSize(double s)            { m_fontSize = s; }

void TextShape::syncBoundsToText()
{
    Transform t = transform();
    const QFont font(m_fontFamily, static_cast<int>(m_fontSize));
    QFontMetrics fm(font);
    const QRect br = fm.boundingRect(QRect(0, 0, static_cast<int>(qMax(t.width, 40.0)), 10000),
                                       Qt::TextWordWrap, m_text);
    t.height = qMax(static_cast<double>(br.height()), fm.height() * 1.2);
    setTransform(t);
}

QJsonObject TextShape::typeSpecificToJson() const
{
    QJsonObject obj;
    obj["text"]       = m_text;
    obj["fontFamily"] = m_fontFamily;
    obj["fontSize"]   = m_fontSize;
    return obj;
}

void TextShape::typeSpecificFromJson(const QJsonObject &obj)
{
    m_text       = obj["text"].toString("Text");
    m_fontFamily = obj["fontFamily"].toString("sans-serif");
    m_fontSize   = obj["fontSize"].toDouble(24.0);
}
