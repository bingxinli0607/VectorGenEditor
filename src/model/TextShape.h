#ifndef TEXTSHAPE_H
#define TEXTSHAPE_H

#include "Shape.h"

class TextShape : public Shape
{
public:
    QString shapeType() const override { return "Text"; }
    std::unique_ptr<Shape> clone() const override;

    QString text() const { return m_text; }
    void setText(const QString &t);

    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString &f);
    double fontSize() const { return m_fontSize; }
    void setFontSize(double s);

    /// Recompute transform height from font metrics and text width.
    void syncBoundsToText();

protected:
    QJsonObject typeSpecificToJson() const override;
    void typeSpecificFromJson(const QJsonObject &obj) override;

private:
    QString m_text = "Text";
    QString m_fontFamily = "sans-serif";
    double  m_fontSize = 24.0;
};

#endif // TEXTSHAPE_H
