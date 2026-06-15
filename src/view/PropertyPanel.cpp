#include "PropertyPanel.h"
#include "controller/DocumentController.h"
#include "command/ModifyShapeCommand.h"
#include "command/ChangeTextCommand.h"
#include "model/Document.h"
#include "model/Layer.h"
#include "model/Shape.h"
#include "model/TextShape.h"
#include "model/Workspace.h"

#include <QUndoStack>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QFontComboBox>
#include <QPushButton>
#include <QColorDialog>

namespace {

QString shapeTypeDisplayName(const QString &type)
{
    if (type == QStringLiteral("Rect")) return QStringLiteral("矩形");
    if (type == QStringLiteral("Ellipse")) return QStringLiteral("椭圆");
    if (type == QStringLiteral("Polygon")) return QStringLiteral("多边形");
    if (type == QStringLiteral("Polyline")) return QStringLiteral("折线");
    if (type == QStringLiteral("Line")) return QStringLiteral("直线");
    if (type == QStringLiteral("Text")) return QStringLiteral("文本");
    if (type == QStringLiteral("Star")) return QStringLiteral("星形");
    if (type == QStringLiteral("Arrow")) return QStringLiteral("箭头");
    return type;
}

QString shapeTypeCodePrefix(const QString &type)
{
    if (type == QStringLiteral("Rect")) return QStringLiteral("Rect");
    if (type == QStringLiteral("Ellipse")) return QStringLiteral("Ellipse");
    if (type == QStringLiteral("Polygon")) return QStringLiteral("Polygon");
    if (type == QStringLiteral("Polyline")) return QStringLiteral("Polyline");
    if (type == QStringLiteral("Line")) return QStringLiteral("Line");
    if (type == QStringLiteral("Text")) return QStringLiteral("Text");
    if (type == QStringLiteral("Star")) return QStringLiteral("Star");
    if (type == QStringLiteral("Arrow")) return QStringLiteral("Arrow");
    return QStringLiteral("Shape");
}

QString shortShapeId(const QString &id)
{
    if (id.size() <= 14) return id;
    return id.left(4) + QStringLiteral("...") + id.right(4);
}

QString shapeDisplayCode(Document *doc, Shape *shape)
{
    if (!doc || !shape) return QStringLiteral("—");

    const QString prefix = shapeTypeCodePrefix(shape->shapeType());
    int index = 0;
    for (const auto &layer : doc->layers()) {
        for (const auto &s : layer->shapes()) {
            if (s->shapeType() == shape->shapeType())
                ++index;
            if (s->id() == shape->id())
                return QStringLiteral("%1-%2").arg(prefix).arg(index, 3, 10, QChar('0'));
        }
    }
    return prefix + QStringLiteral("-001");
}

QString fillDisplayText(const QString &fill)
{
    if (fill == QStringLiteral("none") || fill.isEmpty())
        return QStringLiteral("无");
    return fill;
}

QFormLayout *makeForm(QWidget *parent, int spacing = 10)
{
    auto *form = new QFormLayout();
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(spacing);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    Q_UNUSED(parent);
    return form;
}

} // namespace

PropertyPanel::PropertyPanel(DocumentController *controller, QWidget *parent)
    : QWidget(parent), m_controller(controller)
{
    buildUI();
    connectEditors();
    setInputsEnabled(false);
    setContentVisible(false);

    connect(m_controller->document(), &Document::shapeChanged,
            this, [this](const QString &id) {
        if (m_shape && m_shape->id() == id)
            populateFromShape();
    });
    connect(m_controller->undoStack(), &QUndoStack::indexChanged,
            this, [this]() {
        if (!m_shape) return;
        if (m_controller->document()->findShape(m_shape->id()))
            populateFromShape();
        else
            clearShape();
    });
}

void PropertyPanel::buildUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    auto *panelTitle = new QLabel(QStringLiteral("对象属性"));
    panelTitle->setObjectName("panelTitle");
    mainLayout->addWidget(panelTitle);

    m_emptyLabel = new QLabel(QStringLiteral("未选中对象"));
    m_emptyLabel->setObjectName("emptyHint");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_emptyLabel);

    m_emptySubLabel = new QLabel(QStringLiteral("请在画布中选择一个图形"));
    m_emptySubLabel->setObjectName("emptySubHint");
    m_emptySubLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_emptySubLabel);

    m_multiLabel = new QLabel();
    m_multiLabel->setObjectName("multiHint");
    m_multiLabel->setAlignment(Qt::AlignCenter);
    m_multiLabel->setVisible(false);
    mainLayout->addWidget(m_multiLabel);

    m_contentWidget = new QWidget();
    auto *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(12);

    auto *objectGroup = new QGroupBox(QStringLiteral("当前对象"));
    objectGroup->setToolTip(QStringLiteral("选中对象的类型与编号；完整 ID 见编号 tooltip"));
    auto *objectForm = makeForm(objectGroup, 8);
    m_objectTitleLabel = new QLabel(QStringLiteral("—"));
    m_objectTitleLabel->setObjectName("objectTitle");
    m_codeLabel = new QLabel(QStringLiteral("—"));
    m_codeLabel->setObjectName("codeLabel");
    objectForm->addRow(QStringLiteral("类型"), m_objectTitleLabel);
    objectForm->addRow(QStringLiteral("编号"), m_codeLabel);
    objectGroup->setLayout(objectForm);
    contentLayout->addWidget(objectGroup);

    auto makeSpin = [this](double min, double max, int decimals = 1) {
        auto *s = new QDoubleSpinBox(this);
        s->setRange(min, max);
        s->setDecimals(decimals);
        s->setSingleStep(decimals == 0 ? 1.0 : 0.5);
        s->setButtonSymbols(QAbstractSpinBox::NoButtons);
        s->setMinimumHeight(28);
        return s;
    };

    auto *posGroup = new QGroupBox(QStringLiteral("位置"));
    posGroup->setToolTip(QStringLiteral("修改后按回车或切换焦点生效，支持撤销"));
    auto *posForm = makeForm(posGroup);
    m_xSpin = makeSpin(-10000, 10000);
    m_ySpin = makeSpin(-10000, 10000);
    m_xSpin->setToolTip(QStringLiteral("水平位置 (X)"));
    m_ySpin->setToolTip(QStringLiteral("垂直位置 (Y)"));
    posForm->addRow(QStringLiteral("X"), m_xSpin);
    posForm->addRow(QStringLiteral("Y"), m_ySpin);
    posGroup->setLayout(posForm);
    contentLayout->addWidget(posGroup);

    auto *sizeGroup = new QGroupBox(QStringLiteral("尺寸"));
    auto *sizeForm = makeForm(sizeGroup);
    m_wSpin = makeSpin(10, 10000);
    m_hSpin = makeSpin(10, 10000);
    m_wSpin->setToolTip(QStringLiteral("宽度"));
    m_hSpin->setToolTip(QStringLiteral("高度"));
    sizeForm->addRow(QStringLiteral("宽度"), m_wSpin);
    sizeForm->addRow(QStringLiteral("高度"), m_hSpin);
    sizeGroup->setLayout(sizeForm);
    contentLayout->addWidget(sizeGroup);

    auto *transformGroup = new QGroupBox(QStringLiteral("变换"));
    auto *transformForm = makeForm(transformGroup);
    m_rotSpin = makeSpin(-360, 360, 1);
    m_rotSpin->setToolTip(QStringLiteral("旋转角度（度）"));
    transformForm->addRow(QStringLiteral("角度"), m_rotSpin);
    transformGroup->setLayout(transformForm);
    contentLayout->addWidget(transformGroup);

    auto *appearanceGroup = new QGroupBox(QStringLiteral("外观"));
    appearanceGroup->setToolTip(QStringLiteral("填充、描边与可见性"));
    auto *appearanceForm = makeForm(appearanceGroup);
    m_fillEdit = new QLineEdit();
    m_strokeEdit = new QLineEdit();
    m_strokeWSpin = makeSpin(0, 100, 1);
    m_visibleCb = new QCheckBox(QStringLiteral("可见"));
    m_fillEdit->setMinimumHeight(28);
    m_strokeEdit->setMinimumHeight(28);
    m_fillEdit->setToolTip(QStringLiteral("填充颜色，如 #RRGGBB 或 none"));
    m_strokeEdit->setToolTip(QStringLiteral("描边颜色"));
    m_strokeWSpin->setToolTip(QStringLiteral("线宽"));
    appearanceForm->addRow(QStringLiteral("填充"), m_fillEdit);
    appearanceForm->addRow(QStringLiteral("描边"), m_strokeEdit);
    appearanceForm->addRow(QStringLiteral("线宽"), m_strokeWSpin);
    appearanceForm->addRow(QString(), m_visibleCb);
    appearanceGroup->setLayout(appearanceForm);
    contentLayout->addWidget(appearanceGroup);

    m_textGroup = new QGroupBox(QStringLiteral("文本"));
    m_textGroup->setToolTip(QStringLiteral("文本内容、字体、字号与颜色；双击画布文字也可编辑"));
    auto *textForm = makeForm(m_textGroup);
    m_textEdit = new QLineEdit();
    m_fontCombo = new QFontComboBox();
    m_fontSizeSpin = new QSpinBox();
    m_fontSizeSpin->setRange(8, 96);
    m_fontSizeSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_textColorBtn = new QPushButton(QStringLiteral("选择颜色"));
    m_textEdit->setMinimumHeight(28);
    m_fontCombo->setMinimumHeight(28);
    m_fontSizeSpin->setMinimumHeight(28);
    m_textColorBtn->setMinimumHeight(28);
    textForm->addRow(QStringLiteral("内容"), m_textEdit);
    textForm->addRow(QStringLiteral("字体"), m_fontCombo);
    textForm->addRow(QStringLiteral("字号"), m_fontSizeSpin);
    textForm->addRow(QStringLiteral("颜色"), m_textColorBtn);
    m_textGroup->setLayout(textForm);
    m_textGroup->setVisible(false);
    contentLayout->addWidget(m_textGroup);

    contentLayout->addStretch();
    m_contentWidget->setVisible(false);
    mainLayout->addWidget(m_contentWidget, 1);
}

void PropertyPanel::connectEditors()
{
    const auto hookSpin = [this](QDoubleSpinBox *spin) {
        connect(spin, &QDoubleSpinBox::editingFinished, this, &PropertyPanel::commitChanges);
    };

    hookSpin(m_xSpin);
    hookSpin(m_ySpin);
    hookSpin(m_wSpin);
    hookSpin(m_hSpin);
    hookSpin(m_rotSpin);
    hookSpin(m_strokeWSpin);

    connect(m_fillEdit, &QLineEdit::editingFinished, this, &PropertyPanel::commitChanges);
    connect(m_strokeEdit, &QLineEdit::editingFinished, this, &PropertyPanel::commitChanges);
    connect(m_visibleCb, &QCheckBox::toggled, this, &PropertyPanel::commitChanges);

    connect(m_textEdit, &QLineEdit::editingFinished, this, &PropertyPanel::commitTextChanges);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &) {
        if (!m_populating)
            commitTextChanges();
    });
    connect(m_fontSizeSpin, &QSpinBox::editingFinished, this, &PropertyPanel::commitTextChanges);
    connect(m_textColorBtn, &QPushButton::clicked, this, [this]() {
        if (!m_shape || m_populating) return;
        const QColor current(m_textColor);
        const QColor picked = QColorDialog::getColor(
            current.isValid() ? current : QColor(QStringLiteral("#333333")),
            this, QStringLiteral("文本颜色"));
        if (!picked.isValid())
            return;
        m_textColor = picked.name();
        updateTextColorButton();
        commitTextChanges();
    });
}

void PropertyPanel::updateTextColorButton()
{
    const QColor c(m_textColor);
    const QString bg = c.isValid() ? c.name() : QStringLiteral("#333333");
    const int lum = (c.red() * 299 + c.green() * 587 + c.blue() * 114) / 1000;
    const QString fg = lum > 140 ? QStringLiteral("#222222") : QStringLiteral("#FFFFFF");
    m_textColorBtn->setStyleSheet(
        QStringLiteral("background-color: %1; color: %2; border: 1px solid #D9DEE7; border-radius: 4px;")
            .arg(bg, fg));
    m_textColorBtn->setText(bg);
}

void PropertyPanel::setContentVisible(bool visible)
{
    m_contentWidget->setVisible(visible);
}

void PropertyPanel::setShape(Shape *shape)
{
    m_shape = shape;
    const bool hasShape = shape != nullptr;
    m_emptyLabel->setVisible(!hasShape);
    m_emptySubLabel->setVisible(!hasShape);
    m_multiLabel->setVisible(false);
    setContentVisible(hasShape);
    setInputsEnabled(hasShape);
    if (hasShape)
        populateFromShape();
}

void PropertyPanel::setMultiSelection(int count)
{
    m_shape = nullptr;
    m_emptyLabel->setVisible(false);
    m_emptySubLabel->setVisible(false);
    m_multiLabel->setText(QStringLiteral("已选择 %1 个对象").arg(count));
    m_multiLabel->setVisible(true);
    setContentVisible(false);
    setInputsEnabled(false);
}

void PropertyPanel::clearShape()
{
    setShape(nullptr);
}

void PropertyPanel::populateFromShape()
{
    m_populating = true;

    if (!m_shape) {
        m_emptyLabel->setVisible(true);
        m_emptySubLabel->setVisible(true);
        m_multiLabel->setVisible(false);
        setContentVisible(false);
        m_populating = false;
        return;
    }

    m_emptyLabel->setVisible(false);
    m_emptySubLabel->setVisible(false);
    m_multiLabel->setVisible(false);
    setContentVisible(true);

    const QString typeName = shapeTypeDisplayName(m_shape->shapeType());
    m_objectTitleLabel->setText(typeName);

    const QString code = shapeDisplayCode(m_controller->document(), m_shape);
    m_codeLabel->setText(code);
    m_codeLabel->setToolTip(QStringLiteral("完整 ID：%1").arg(m_shape->id()));
    m_codeLabel->setStatusTip(shortShapeId(m_shape->id()));

    const bool isText = m_shape->shapeType() == QStringLiteral("Text");
    m_textGroup->setVisible(isText);

    Transform t = m_shape->transform();
    {
        QSignalBlocker b1(m_xSpin), b2(m_ySpin), b3(m_wSpin), b4(m_hSpin), b5(m_rotSpin);
        m_xSpin->setValue(t.x);
        m_ySpin->setValue(t.y);
        m_wSpin->setValue(t.width);
        m_hSpin->setValue(t.height);
        m_rotSpin->setValue(t.rotation);
    }

    Style s = m_shape->style();
    {
        QSignalBlocker b1(m_fillEdit), b2(m_strokeEdit), b3(m_strokeWSpin), b4(m_visibleCb);
        m_fillEdit->setText(fillDisplayText(s.fillColor));
        m_strokeEdit->setText(s.strokeColor);
        m_strokeWSpin->setValue(s.strokeWidth);
        m_visibleCb->setChecked(s.visible);
    }

    if (auto *txt = dynamic_cast<TextShape*>(m_shape)) {
        QSignalBlocker b1(m_textEdit), b2(m_fontCombo), b3(m_fontSizeSpin);
        m_textEdit->setText(txt->text());
        QFont font;
        font.setFamily(txt->fontFamily());
        m_fontCombo->setCurrentFont(font);
        m_fontSizeSpin->setValue(qBound(8, static_cast<int>(qRound(txt->fontSize())), 96));
        m_textColor = txt->style().strokeColor;
        updateTextColorButton();
    }

    m_populating = false;
}

void PropertyPanel::setInputsEnabled(bool enabled)
{
    m_xSpin->setEnabled(enabled);
    m_ySpin->setEnabled(enabled);
    m_wSpin->setEnabled(enabled);
    m_hSpin->setEnabled(enabled);
    m_rotSpin->setEnabled(enabled);
    m_fillEdit->setEnabled(enabled);
    m_strokeEdit->setEnabled(enabled);
    m_strokeWSpin->setEnabled(enabled);
    m_visibleCb->setEnabled(enabled);
    m_objectTitleLabel->setEnabled(enabled);
    m_codeLabel->setEnabled(enabled);
    m_textEdit->setEnabled(enabled);
    m_fontCombo->setEnabled(enabled);
    m_fontSizeSpin->setEnabled(enabled);
    m_textColorBtn->setEnabled(enabled);
}

void PropertyPanel::commitChanges()
{
    if (!m_shape || !m_controller || m_populating) return;

    Transform oldT = m_shape->transform();
    Transform newT = {
        m_xSpin->value(),
        m_ySpin->value(),
        qMax(10.0, m_wSpin->value()),
        qMax(10.0, m_hSpin->value()),
        m_rotSpin->value()
    };
    newT = Workspace::clampTransform(*m_shape, newT, m_controller->document()->workspaceRect());

    Style oldS = m_shape->style();
    Style newS = oldS;

    QString fillText = m_fillEdit->text().trimmed();
    if (fillText == QStringLiteral("无"))
        newS.fillColor = QStringLiteral("none");
    else if (!fillText.isEmpty())
        newS.fillColor = fillText;

    QString strokeText = m_strokeEdit->text().trimmed();
    if (!strokeText.isEmpty())
        newS.strokeColor = strokeText;

    newS.strokeWidth = m_strokeWSpin->value();
    newS.visible     = m_visibleCb->isChecked();

    if (oldT.x == newT.x && oldT.y == newT.y && oldT.width == newT.width
        && oldT.height == newT.height && oldT.rotation == newT.rotation
        && oldS.fillColor == newS.fillColor && oldS.strokeColor == newS.strokeColor
        && oldS.strokeWidth == newS.strokeWidth && oldS.visible == newS.visible) {
        return;
    }

    m_controller->undoStack()->push(
        new ModifyShapeCommand(m_controller->document(), m_shape->id(),
                               oldT, newT, oldS, newS));

    {
        QSignalBlocker b1(m_xSpin), b2(m_ySpin), b3(m_wSpin), b4(m_hSpin), b5(m_rotSpin);
        m_xSpin->setValue(newT.x);
        m_ySpin->setValue(newT.y);
        m_wSpin->setValue(newT.width);
        m_hSpin->setValue(newT.height);
        m_rotSpin->setValue(newT.rotation);
    }
}

void PropertyPanel::commitTextChanges()
{
    if (!m_shape || !m_controller || m_populating) return;
    auto *txt = dynamic_cast<TextShape*>(m_shape);
    if (!txt) return;

    TextProps oldP{
        txt->text(),
        txt->fontFamily(),
        txt->fontSize(),
        txt->style().strokeColor
    };
    TextProps newP{
        m_textEdit->text(),
        m_fontCombo->currentFont().family(),
        static_cast<double>(m_fontSizeSpin->value()),
        m_textColor
    };

    if (oldP.text == newP.text && oldP.fontFamily == newP.fontFamily
        && qFuzzyCompare(oldP.fontSize, newP.fontSize)
        && oldP.textColor == newP.textColor) {
        return;
    }

    m_controller->updateShapeText(m_shape->id(), oldP, newP);
}
