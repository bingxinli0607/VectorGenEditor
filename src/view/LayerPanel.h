#ifndef LAYERPANEL_H
#define LAYERPANEL_H

#include <QWidget>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;
class Document;
class DocumentController;

class LayerPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LayerPanel(DocumentController *controller,
                        QWidget *parent = nullptr);

private slots:
    void onAddLayer();
    void onRemoveLayer();
    void onRenameLayer();
    void onSelectionChanged(int row);
    void onItemChanged(QListWidgetItem *item);
    void refreshFromDocument();

private:
    void buildUI();

    DocumentController *m_controller = nullptr;
    QListWidget *m_listWidget = nullptr;
    QLabel *m_statsLabel = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_removeBtn = nullptr;
    QPushButton *m_renameBtn = nullptr;
};

#endif // LAYERPANEL_H
