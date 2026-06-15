#ifndef GENERATIONPANEL_H
#define GENERATIONPANEL_H

#include <QWidget>

class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class DocumentController;

class GenerationPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GenerationPanel(DocumentController *controller,
                             QWidget *parent = nullptr);

public slots:
    void triggerGenerateStar();
    void triggerGenerateArrow();
    void triggerGenerateArray();

private slots:
    void onGenerate();

private:
    void buildUI();

    DocumentController *m_controller = nullptr;

    QSpinBox *m_starPoints = nullptr;
    QDoubleSpinBox *m_starOuterR = nullptr;
    QDoubleSpinBox *m_starInnerRatio = nullptr;
    QSpinBox *m_starCx = nullptr;
    QSpinBox *m_starCy = nullptr;

    QSpinBox *m_arrowStartX = nullptr;
    QSpinBox *m_arrowStartY = nullptr;
    QSpinBox *m_arrowEndX = nullptr;
    QSpinBox *m_arrowEndY = nullptr;

    QSpinBox *m_arrayCols = nullptr;
    QSpinBox *m_arrayRows = nullptr;
    QDoubleSpinBox *m_arraySpacingX = nullptr;
    QDoubleSpinBox *m_arraySpacingY = nullptr;
    QComboBox *m_arrayShapeType = nullptr;

    QPushButton *m_genStarBtn = nullptr;
    QPushButton *m_genArrowBtn = nullptr;
    QPushButton *m_genArrayBtn = nullptr;
};

#endif // GENERATIONPANEL_H
