#ifndef TOOLPALETTE_H
#define TOOLPALETTE_H

#include <QFrame>
#include <QHash>
#include <QSize>

class QAbstractButton;
class QButtonGroup;

enum class PaletteToolType {
    Select,
    Line,
    Rect,
    Ellipse,
    Triangle,
    Pentagon,
    Polyline,
    Text,
};

class ToolPalette : public QFrame
{
    Q_OBJECT

public:
    explicit ToolPalette(QWidget *parent = nullptr);

    PaletteToolType currentTool() const { return m_currentTool; }
    void setCurrentTool(PaletteToolType tool);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void toolSelected(PaletteToolType tool);

private:
    void buildUi();
    static QIcon iconForTool(PaletteToolType type);

    QButtonGroup *m_buttonGroup = nullptr;
    QHash<QAbstractButton*, PaletteToolType> m_buttonTools;
    PaletteToolType m_currentTool = PaletteToolType::Select;
};

#endif // TOOLPALETTE_H
