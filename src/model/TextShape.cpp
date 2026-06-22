#include "TextShape.h"
#include <QFontMetrics>
#include <QTextLayout>

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
    const QFontMetricsF fm(font);

    if (m_text.isEmpty()) {
        t.width  = 20.0;
        t.height = fm.height() * 1.2;
        setTransform(t);
        return;
    }

    // ----------------------------------------------------------------
    // Use QTextLayout (Qt's core text-layout engine) for every
    // measurement.  QPainter::drawText() delegates to the same engine
    // internally, so width & height agree pixel-for-pixel across all
    // scripts, continuous Latin strings, punctuation, emoji, etc.
    // ----------------------------------------------------------------
    static constexpr double kUnlimited = 50000.0;
    static constexpr double kPadding   = 14.0;

    // --- Width: measure each explicit line at unlimited width.
    //     QTextLayout::boundingRect() includes the full line-width
    //     (kUnlimited), which would blow the box to canvas size.
    //     Use QTextLine::naturalTextRect() instead — it returns the
    //     tight pixel bounds of the rendered glyphs on that line. ---
    const QStringList lines = m_text.split('\n');
    double bestW = 0.0;
    for (const QString &line : lines) {
        if (line.isEmpty()) continue;

        QTextLayout widthLayout(line, font);
        widthLayout.beginLayout();
        QTextLine wl = widthLayout.createLine();
        if (wl.isValid())
            wl.setLineWidth(kUnlimited);
        widthLayout.endLayout();

        // naturalTextWidth()  = advance width (cursor pos after last char)
        // naturalTextRect().right() = tight glyph-pixel edge
        // Use the max so both right-bearing and italic overhang are covered.
        bestW = qMax(bestW, qMax(wl.naturalTextWidth(),
                                 wl.naturalTextRect().right()));
    }

    t.width = qMax(bestW + kPadding, 20.0);

    // --- Height: measure full text word-wrapped at the new width ---
    QTextLayout hLayout(m_text, font);
    {
        QTextOption opt;
        opt.setWrapMode(QTextOption::WordWrap);
        opt.setAlignment(Qt::AlignLeft);
        hLayout.setTextOption(opt);
    }

    hLayout.beginLayout();
    double totalH = 0.0;
    while (true) {
        QTextLine hl = hLayout.createLine();
        if (!hl.isValid()) break;
        hl.setLineWidth(t.width);
        totalH += hl.height();
    }
    hLayout.endLayout();

    t.height = qMax(totalH, fm.height() * 1.2);
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
