#ifndef PROPERTYPANEL_H
#define PROPERTYPANEL_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QGroupBox;
class QFontComboBox;
class QPushButton;
class Shape;
class DocumentController;

class PropertyPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyPanel(DocumentController *controller,
                            QWidget *parent = nullptr);

    void setShape(Shape *shape);
    void setMultiSelection(int count);
    void clearShape();

private slots:
    void commitChanges();
    void commitTextChanges();

private:
    void buildUI();
    void populateFromShape();
    void setInputsEnabled(bool enabled);
    void setContentVisible(bool visible);
    void connectEditors();
    void updateTextColorButton();

    DocumentController *m_controller = nullptr;
    Shape *m_shape = nullptr;
    bool m_populating = false;

    QWidget *m_contentWidget = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QLabel *m_emptySubLabel = nullptr;
    QLabel *m_multiLabel = nullptr;

    QLabel *m_objectTitleLabel = nullptr;
    QLabel *m_codeLabel = nullptr;

    QDoubleSpinBox *m_xSpin = nullptr;
    QDoubleSpinBox *m_ySpin = nullptr;
    QDoubleSpinBox *m_wSpin = nullptr;
    QDoubleSpinBox *m_hSpin = nullptr;
    QDoubleSpinBox *m_rotSpin = nullptr;
    QLineEdit *m_fillEdit = nullptr;
    QLineEdit *m_strokeEdit = nullptr;
    QDoubleSpinBox *m_strokeWSpin = nullptr;
    QCheckBox  *m_visibleCb = nullptr;

    QGroupBox *m_textGroup = nullptr;
    QLineEdit *m_textEdit = nullptr;
    QFontComboBox *m_fontCombo = nullptr;
    QSpinBox *m_fontSizeSpin = nullptr;
    QPushButton *m_textColorBtn = nullptr;
    QString m_textColor;
};

#endif // PROPERTYPANEL_H
