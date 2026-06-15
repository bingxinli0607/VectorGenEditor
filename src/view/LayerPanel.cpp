#include "LayerPanel.h"
#include "controller/DocumentController.h"
#include "view/CanvasScene.h"
#include "model/Document.h"
#include "model/Layer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>

LayerPanel::LayerPanel(DocumentController *controller, QWidget *parent)
    : QWidget(parent), m_controller(controller)
{
    buildUI();
    refreshFromDocument();

    connect(m_controller->document(), &Document::layerChanged,
            this, &LayerPanel::refreshFromDocument);
    connect(m_controller->document(), &Document::documentChanged,
            this, &LayerPanel::refreshFromDocument);
}

void LayerPanel::buildUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto *title = new QLabel(QStringLiteral("图层管理"));
    title->setObjectName("panelTitle");
    title->setToolTip(QStringLiteral("勾选控制图层可见性；双击列表项可切换当前图层"));
    layout->addWidget(title);

    m_statsLabel = new QLabel();
    m_statsLabel->setStyleSheet(QStringLiteral("color: #666666; font-size: 11pt;"));
    layout->addWidget(m_statsLabel);

    m_listWidget = new QListWidget();
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setToolTip(QStringLiteral("勾选控制图层可见性；双击列表项可切换当前图层"));
    connect(m_listWidget, &QListWidget::currentRowChanged,
            this, &LayerPanel::onSelectionChanged);
    connect(m_listWidget, &QListWidget::itemChanged,
            this, &LayerPanel::onItemChanged);
    layout->addWidget(m_listWidget, 1);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_addBtn = new QPushButton(QStringLiteral("新建"));
    m_addBtn->setObjectName("secondaryBtn");
    m_addBtn->setMinimumHeight(32);
    m_addBtn->setToolTip(QStringLiteral("新建图层"));
    connect(m_addBtn, &QPushButton::clicked, this, &LayerPanel::onAddLayer);
    btnLayout->addWidget(m_addBtn);

    m_removeBtn = new QPushButton(QStringLiteral("删除"));
    m_removeBtn->setObjectName("secondaryBtn");
    m_removeBtn->setMinimumHeight(32);
    m_removeBtn->setToolTip(QStringLiteral("删除当前图层"));
    connect(m_removeBtn, &QPushButton::clicked, this, &LayerPanel::onRemoveLayer);
    btnLayout->addWidget(m_removeBtn);

    m_renameBtn = new QPushButton(QStringLiteral("重命名"));
    m_renameBtn->setObjectName("secondaryBtn");
    m_renameBtn->setMinimumHeight(32);
    m_renameBtn->setToolTip(QStringLiteral("重命名图层"));
    connect(m_renameBtn, &QPushButton::clicked, this, &LayerPanel::onRenameLayer);
    btnLayout->addWidget(m_renameBtn);

    layout->addLayout(btnLayout);
}

void LayerPanel::refreshFromDocument()
{
    auto *doc = m_controller->document();
    if (!doc) return;

    int shapeCount = 0;
    for (const auto &layer : doc->layers())
        shapeCount += static_cast<int>(layer->shapes().size());

    m_statsLabel->setText(QStringLiteral("图层 %1  ·  图形 %2")
                              .arg(doc->layerCount())
                              .arg(shapeCount));

    m_listWidget->blockSignals(true);
    m_listWidget->clear();

    for (int i = 0; i < doc->layerCount(); ++i) {
        auto *layer = doc->layerAt(i);
        if (!layer) continue;
        const QString label = QStringLiteral("%1  (%2)")
                                  .arg(layer->name())
                                  .arg(layer->shapes().size());
        auto *item = new QListWidgetItem(label);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setCheckState(layer->visible() ? Qt::Checked : Qt::Unchecked);
        m_listWidget->addItem(item);
    }

    const int current = doc->currentLayerIndex();
    if (current >= 0 && current < m_listWidget->count())
        m_listWidget->setCurrentRow(current);

    m_listWidget->blockSignals(false);
    m_removeBtn->setEnabled(doc->layerCount() > 1);
}

void LayerPanel::onSelectionChanged(int row)
{
    auto *doc = m_controller->document();
    if (doc && row >= 0 && row < doc->layerCount())
        doc->setCurrentLayer(row);
}

void LayerPanel::onItemChanged(QListWidgetItem *item)
{
    if (!item) return;
    auto *doc = m_controller->document();
    const int row = m_listWidget->row(item);
    if (!doc || row < 0 || row >= doc->layerCount()) return;

    auto *layer = doc->layerAt(row);
    if (!layer) return;

    const bool visible = item->checkState() == Qt::Checked;
    if (layer->visible() != visible) {
        layer->setVisible(visible);
        if (m_controller->scene())
            m_controller->scene()->syncLayerVisibility();
    }
}

void LayerPanel::onAddLayer()
{
    bool ok;
    QString name = QInputDialog::getText(this, QStringLiteral("新建图层"),
                                          QStringLiteral("图层名称:"),
                                          QLineEdit::Normal,
                                          QStringLiteral("图层 %1").arg(m_controller->document()->layerCount() + 1),
                                          &ok);
    if (ok && !name.isEmpty())
        m_controller->document()->addLayer(name);
}

void LayerPanel::onRemoveLayer()
{
    auto *doc = m_controller->document();
    if (doc->layerCount() <= 1) {
        QMessageBox::information(this, QStringLiteral("无法删除"),
                                  QStringLiteral("至少需要保留一个图层。"));
        return;
    }
    const int row = m_listWidget->currentRow();
    if (row >= 0)
        doc->removeLayer(row);
}

void LayerPanel::onRenameLayer()
{
    const int row = m_listWidget->currentRow();
    auto *doc = m_controller->document();
    if (row < 0 || row >= doc->layerCount()) return;
    auto *layer = doc->layerAt(row);
    if (!layer) return;

    bool ok;
    QString name = QInputDialog::getText(this, QStringLiteral("重命名图层"),
                                          QStringLiteral("图层名称:"),
                                          QLineEdit::Normal, layer->name(), &ok);
    if (ok && !name.isEmpty()) {
        layer->setName(name);
        emit doc->layerChanged();
    }
}
