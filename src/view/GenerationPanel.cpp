#include "GenerationPanel.h"
#include "controller/DocumentController.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>

namespace {

QFormLayout *makeForm()
{
    auto *form = new QFormLayout();
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(10);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    return form;
}

void styleSpin(QSpinBox *spin)
{
    spin->setMinimumHeight(28);
}

void styleDoubleSpin(QDoubleSpinBox *spin)
{
    spin->setMinimumHeight(28);
}

void styleButton(QPushButton *btn)
{
    btn->setMinimumHeight(34);
}

} // namespace

GenerationPanel::GenerationPanel(DocumentController *controller, QWidget *parent)
    : QWidget(parent), m_controller(controller)
{
    buildUI();
}

void GenerationPanel::buildUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);

    auto *starGroup = new QGroupBox(QStringLiteral("星形"));
    starGroup->setToolTip(QStringLiteral("根据角数、半径和中心点生成可编辑星形"));
    auto *starForm = makeForm();

    m_starPoints = new QSpinBox();
    m_starPoints->setRange(3, 20);
    m_starPoints->setValue(5);
    styleSpin(m_starPoints);
    starForm->addRow(QStringLiteral("角数"), m_starPoints);

    m_starOuterR = new QDoubleSpinBox();
    m_starOuterR->setRange(10, 500);
    m_starOuterR->setValue(80);
    m_starOuterR->setDecimals(1);
    styleDoubleSpin(m_starOuterR);
    starForm->addRow(QStringLiteral("外半径"), m_starOuterR);

    m_starInnerRatio = new QDoubleSpinBox();
    m_starInnerRatio->setRange(0.1, 0.9);
    m_starInnerRatio->setValue(0.5);
    m_starInnerRatio->setSingleStep(0.05);
    m_starInnerRatio->setDecimals(2);
    styleDoubleSpin(m_starInnerRatio);
    starForm->addRow(QStringLiteral("内径比"), m_starInnerRatio);

    m_starCx = new QSpinBox();
    m_starCx->setRange(0, 2000);
    m_starCx->setValue(400);
    styleSpin(m_starCx);
    starForm->addRow(QStringLiteral("中心 X"), m_starCx);

    m_starCy = new QSpinBox();
    m_starCy->setRange(0, 2000);
    m_starCy->setValue(300);
    styleSpin(m_starCy);
    starForm->addRow(QStringLiteral("中心 Y"), m_starCy);

    m_genStarBtn = new QPushButton(QStringLiteral("生成星形"));
    styleButton(m_genStarBtn);
    connect(m_genStarBtn, &QPushButton::clicked, this, &GenerationPanel::triggerGenerateStar);
    starForm->addRow(m_genStarBtn);
    starGroup->setLayout(starForm);
    mainLayout->addWidget(starGroup);

    auto *arrowGroup = new QGroupBox(QStringLiteral("箭头"));
    arrowGroup->setToolTip(QStringLiteral("指定起点与终点生成矢量箭头"));
    auto *arrowForm = makeForm();

    m_arrowStartX = new QSpinBox();
    m_arrowStartX->setRange(0, 2000);
    m_arrowStartX->setValue(100);
    styleSpin(m_arrowStartX);
    arrowForm->addRow(QStringLiteral("起点 X"), m_arrowStartX);

    m_arrowStartY = new QSpinBox();
    m_arrowStartY->setRange(0, 2000);
    m_arrowStartY->setValue(100);
    styleSpin(m_arrowStartY);
    arrowForm->addRow(QStringLiteral("起点 Y"), m_arrowStartY);

    m_arrowEndX = new QSpinBox();
    m_arrowEndX->setRange(0, 2000);
    m_arrowEndX->setValue(400);
    styleSpin(m_arrowEndX);
    arrowForm->addRow(QStringLiteral("终点 X"), m_arrowEndX);

    m_arrowEndY = new QSpinBox();
    m_arrowEndY->setRange(0, 2000);
    m_arrowEndY->setValue(300);
    styleSpin(m_arrowEndY);
    arrowForm->addRow(QStringLiteral("终点 Y"), m_arrowEndY);

    m_genArrowBtn = new QPushButton(QStringLiteral("生成箭头"));
    styleButton(m_genArrowBtn);
    connect(m_genArrowBtn, &QPushButton::clicked, this, &GenerationPanel::triggerGenerateArrow);
    arrowForm->addRow(m_genArrowBtn);
    arrowGroup->setLayout(arrowForm);
    mainLayout->addWidget(arrowGroup);

    auto *arrayGroup = new QGroupBox(QStringLiteral("阵列"));
    arrayGroup->setToolTip(QStringLiteral("按行列与间距批量生成矩形、椭圆或星形"));
    auto *arrayForm = makeForm();

    m_arrayShapeType = new QComboBox();
    m_arrayShapeType->addItems({QStringLiteral("矩形"), QStringLiteral("椭圆"), QStringLiteral("星形")});
    m_arrayShapeType->setMinimumHeight(28);
    arrayForm->addRow(QStringLiteral("类型"), m_arrayShapeType);

    m_arrayCols = new QSpinBox();
    m_arrayCols->setRange(1, 20);
    m_arrayCols->setValue(3);
    styleSpin(m_arrayCols);
    arrayForm->addRow(QStringLiteral("列数"), m_arrayCols);

    m_arrayRows = new QSpinBox();
    m_arrayRows->setRange(1, 20);
    m_arrayRows->setValue(3);
    styleSpin(m_arrayRows);
    arrayForm->addRow(QStringLiteral("行数"), m_arrayRows);

    m_arraySpacingX = new QDoubleSpinBox();
    m_arraySpacingX->setRange(20, 500);
    m_arraySpacingX->setValue(120);
    m_arraySpacingX->setDecimals(1);
    styleDoubleSpin(m_arraySpacingX);
    arrayForm->addRow(QStringLiteral("间距 X"), m_arraySpacingX);

    m_arraySpacingY = new QDoubleSpinBox();
    m_arraySpacingY->setRange(20, 500);
    m_arraySpacingY->setValue(120);
    m_arraySpacingY->setDecimals(1);
    styleDoubleSpin(m_arraySpacingY);
    arrayForm->addRow(QStringLiteral("间距 Y"), m_arraySpacingY);

    m_genArrayBtn = new QPushButton(QStringLiteral("生成阵列"));
    styleButton(m_genArrayBtn);
    connect(m_genArrayBtn, &QPushButton::clicked, this, &GenerationPanel::triggerGenerateArray);
    arrayForm->addRow(m_genArrayBtn);
    arrayGroup->setLayout(arrayForm);
    mainLayout->addWidget(arrayGroup);

    mainLayout->addStretch();
}

void GenerationPanel::triggerGenerateStar()
{
    m_controller->createStar(
        m_starCx->value(), m_starCy->value(),
        m_starOuterR->value(), m_starPoints->value(),
        m_starInnerRatio->value());
}

void GenerationPanel::triggerGenerateArrow()
{
    m_controller->createArrow(
        QPointF(m_arrowStartX->value(), m_arrowStartY->value()),
        QPointF(m_arrowEndX->value(), m_arrowEndY->value()));
}

void GenerationPanel::triggerGenerateArray()
{
    onGenerate();
}

void GenerationPanel::onGenerate()
{
    const int cols = m_arrayCols->value();
    const int rows = m_arrayRows->value();
    const double spx = m_arraySpacingX->value();
    const double spy = m_arraySpacingY->value();
    const QString type = m_arrayShapeType->currentText();
    const double baseX = 200.0;
    const double baseY = 100.0;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const double x = baseX + c * spx;
            const double y = baseY + r * spy;

            if (type == QStringLiteral("矩形")) {
                m_controller->createRect(x, y, 80, 60);
            } else if (type == QStringLiteral("椭圆")) {
                m_controller->createEllipse(x, y, 80, 60);
            } else if (type == QStringLiteral("星形")) {
                m_controller->createStar(x + 40, y + 30, 35, 5, 0.5);
            }
        }
    }
}
