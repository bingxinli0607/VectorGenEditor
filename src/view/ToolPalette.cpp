#include "ToolPalette.h"

#include <QButtonGroup>
#include <QHash>
#include <QAbstractButton>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScreen>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

namespace {

constexpr int kPanelWidth = 88;
constexpr int kIconSize = 32;
constexpr int kButtonSize = 56;
constexpr int kPanelMargin = 12;
constexpr int kButtonSpacing = 8;

qreal paletteDevicePixelRatio()
{
    if (const auto screens = QGuiApplication::screens(); !screens.isEmpty())
        return qMax(1.0, screens.first()->devicePixelRatio());
    return 1.0;
}

QPixmap makeToolPixmap(PaletteToolType type, const QColor &color)
{
    const qreal dpr = paletteDevicePixelRatio();
    QPixmap pm(qRound(kIconSize * dpr), qRound(kIconSize * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QPen pen(color, 2.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const QRectF box(5.0, 5.0, kIconSize - 10.0, kIconSize - 10.0);
    const QPointF c = box.center();

    switch (type) {
    case PaletteToolType::Select: {
        QPainterPath arrow;
        arrow.moveTo(box.left() + 2, box.top() + 2);
        arrow.lineTo(box.left() + 2, box.bottom() - 4);
        arrow.lineTo(box.left() + 10, box.bottom() - 4);
        arrow.lineTo(box.left() + 10, box.bottom() - 10);
        arrow.lineTo(box.right() - 2, box.center().y());
        arrow.lineTo(box.left() + 10, box.top() + 10);
        arrow.lineTo(box.left() + 10, box.top() + 12);
        arrow.closeSubpath();
        p.drawPath(arrow);
        break;
    }
    case PaletteToolType::Line:
        p.drawLine(box.bottomLeft() + QPointF(2, -2), box.topRight() - QPointF(2, 2));
        break;
    case PaletteToolType::Rect:
        p.drawRect(box.adjusted(1, 2, -1, -2));
        break;
    case PaletteToolType::Ellipse:
        p.drawEllipse(box.adjusted(1, 2, -1, -2));
        break;
    case PaletteToolType::Triangle: {
        QPolygonF tri;
        tri << QPointF(c.x(), box.top() + 2)
            << QPointF(box.left() + 2, box.bottom() - 2)
            << QPointF(box.right() - 2, box.bottom() - 2);
        p.drawPolygon(tri);
        break;
    }
    case PaletteToolType::Pentagon: {
        QPolygonF poly;
        for (int i = 0; i < 5; ++i) {
            const double angle = qDegreesToRadians(-90.0 + i * 72.0);
            const double rx = box.width() * 0.42;
            const double ry = box.height() * 0.42;
            poly << QPointF(c.x() + rx * qCos(angle), c.y() + ry * qSin(angle));
        }
        p.drawPolygon(poly);
        break;
    }
    case PaletteToolType::Polyline: {
        QPainterPath path;
        path.moveTo(box.left() + 1, box.bottom() - 3);
        path.lineTo(box.left() + 9, box.top() + 10);
        path.lineTo(box.left() + 17, box.bottom() - 7);
        path.lineTo(box.right() - 1, box.top() + 5);
        p.drawPath(path);
        break;
    }
    case PaletteToolType::Text: {
        QFont font = p.font();
        font.setBold(true);
        font.setPixelSize(22);
        p.setFont(font);
        p.setPen(color);
        p.drawText(box, Qt::AlignCenter, QStringLiteral("T"));
        break;
    }
    }

    return pm;
}

QIcon buildToolIcon(PaletteToolType type)
{
    const QColor normal(0x33, 0x33, 0x33);
    const QColor active(0xff, 0xff, 0xff);
    QIcon icon;
    icon.addPixmap(makeToolPixmap(type, normal), QIcon::Normal, QIcon::Off);
    icon.addPixmap(makeToolPixmap(type, normal), QIcon::Active, QIcon::Off);
    icon.addPixmap(makeToolPixmap(type, active), QIcon::Selected, QIcon::On);
    icon.addPixmap(makeToolPixmap(type, active), QIcon::Active, QIcon::On);
    return icon;
}

struct ToolEntry {
    PaletteToolType type;
    const char *tooltip;
};

const ToolEntry kTools[] = {
    {PaletteToolType::Select,   "选择"},
    {PaletteToolType::Line,     "直线"},
    {PaletteToolType::Rect,     "矩形"},
    {PaletteToolType::Ellipse,  "椭圆"},
    {PaletteToolType::Triangle, "三角"},
    {PaletteToolType::Pentagon, "五边"},
    {PaletteToolType::Polyline, "折线"},
    {PaletteToolType::Text,     "文本"},
};

} // namespace

QIcon ToolPalette::iconForTool(PaletteToolType type)
{
    return buildToolIcon(type);
}

ToolPalette::ToolPalette(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("toolPalette"));
    setMinimumWidth(kPanelWidth);
    setMaximumWidth(kPanelWidth);
    setFixedWidth(kPanelWidth);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    buildUi();
}

QSize ToolPalette::sizeHint() const
{
    return {kPanelWidth, QFrame::sizeHint().height()};
}

QSize ToolPalette::minimumSizeHint() const
{
    return {kPanelWidth, QFrame::minimumSizeHint().height()};
}

void ToolPalette::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(kPanelMargin, kPanelMargin, kPanelMargin, kPanelMargin);
    layout->setSpacing(kButtonSpacing);
    layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    auto *title = new QLabel(QStringLiteral("工具"));
    title->setObjectName(QStringLiteral("toolPaletteTitle"));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title, 0, Qt::AlignHCenter);

    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);

    for (const auto &entry : kTools) {
        auto *btn = new QToolButton(this);
        btn->setObjectName(QStringLiteral("toolPaletteButton"));
        btn->setToolTip(QString::fromUtf8(entry.tooltip));
        btn->setIcon(iconForTool(entry.type));
        btn->setIconSize(QSize(kIconSize, kIconSize));
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setCheckable(true);
        btn->setAutoRaise(false);
        btn->setMinimumSize(kButtonSize, kButtonSize);
        btn->setMaximumSize(kButtonSize, kButtonSize);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        m_buttonGroup->addButton(btn);
        m_buttonTools.insert(btn, entry.type);
        layout->addWidget(btn, 0, Qt::AlignHCenter);

        connect(btn, &QToolButton::clicked, this, [this, btn, type = entry.type]() {
            if (!btn->isChecked())
                return;
            if (m_currentTool == type)
                return;
            m_currentTool = type;
            emit toolSelected(type);
        });
    }

    if (auto *first = m_buttonGroup->buttons().value(0))
        first->setChecked(true);

    layout->addStretch();
}

void ToolPalette::setCurrentTool(PaletteToolType tool)
{
    m_currentTool = tool;
    if (!m_buttonGroup)
        return;

    for (auto *btn : m_buttonGroup->buttons()) {
        const bool match = m_buttonTools.value(btn) == tool;
        if (btn->isChecked() != match)
            btn->setChecked(match);
    }
}
