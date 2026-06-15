#include "MainWindow.h"
#include "view/CanvasView.h"
#include "CanvasScene.h"
#include "ShapeGraphicsItem.h"
#include "PropertyPanel.h"
#include "LayerPanel.h"
#include "GenerationPanel.h"
#include "UiStyles.h"
#include "model/Document.h"
#include "model/Layer.h"
#include "serialization/JsonSerializer.h"
#include "serialization/SvgExporter.h"
#include "controller/DocumentController.h"
#include "controller/SelectionController.h"
#include "model/Shape.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QSplitter>
#include <QFrame>
#include <QLabel>
#include <QAction>
#include <QActionGroup>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QPainter>
#include <QKeyEvent>

MainWindow::MainWindow(DocumentController *controller, QWidget *parent)
    : QMainWindow(parent), m_controller(controller)
{
    setWindowTitle(QStringLiteral("VectorGenEditor — 参数化矢量图形生成与编辑器"));
    resize(1600, 900);
    setMinimumSize(1100, 700);

    m_canvasScene = m_controller->scene();
    m_canvasView  = new CanvasView(m_canvasScene, this);
    m_canvasView->setDocumentController(m_controller);
    if (m_canvasScene->selectionController())
        m_canvasView->setSelectionController(m_canvasScene->selectionController());
    m_propertyPanel = new PropertyPanel(m_controller);
    m_generationPanel = new GenerationPanel(m_controller);
    m_layerPanel = new LayerPanel(m_controller);

    applyStyleSheet();
    createMenuBar();
    createMainToolBar();
    createStatusBarWidgets();
    createCentralLayout();

    auto *selCtrl = m_canvasScene->selectionController();
    if (selCtrl) {
        connect(selCtrl, &SelectionController::selectionChanged,
                this, &MainWindow::onSelectionChanged);
        connect(selCtrl, &SelectionController::statusMessage,
                this, [this](const QString &msg) {
            m_statusMessageLabel->setText(msg);
        });
    }

    connect(m_controller->document(), &Document::documentChanged,
            this, &MainWindow::updateStatusBar);
    connect(m_controller->document(), &Document::layerChanged,
            this, &MainWindow::updateStatusBar);
    connect(m_canvasView, &CanvasView::mouseScenePosChanged, this, [this](const QPointF &p) {
        m_statusPosLabel->setText(QStringLiteral("坐标：%1, %2")
                                      .arg(p.x(), 0, 'f', 0)
                                      .arg(p.y(), 0, 'f', 0));
    });
    connect(m_canvasView, &CanvasView::zoomLevelChanged, this, [this](double z) {
        m_statusZoomLabel->setText(QStringLiteral("缩放：%1%").arg(int(z * 100)));
    });

    updateStatusBar();
    onSelectTool();
    m_canvasView->fitWorkspaceDefault();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    auto *sel = m_canvasScene->selectionController();

    if (event->matches(QKeySequence::SelectAll)) {
        if (sel) sel->selectAll();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Delete) || event->key() == Qt::Key_Backspace) {
        deleteSelection();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (sel) sel->deselectAll();
        m_canvasView->cancelActiveOperation();
        onSelectTool();
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

MainWindow::~MainWindow() = default;

void MainWindow::applyStyleSheet()
{
    setStyleSheet(UiStyles::appStyleSheet());
}

void MainWindow::setCurrentTool(const QString &toolName)
{
    m_currentToolName = toolName;
    m_statusToolLabel->setText(QStringLiteral("工具：%1").arg(toolName));
}

void MainWindow::onSelectionChanged()
{
    auto *selCtrl = m_canvasScene->selectionController();
    if (!selCtrl || !selCtrl->hasSelection()) {
        m_propertyPanel->clearShape();
        m_statusSelLabel->setText(QStringLiteral("已选：0"));
        m_statusMessageLabel->setText(QStringLiteral("就绪"));
    } else {
        const int n = selCtrl->selectionCount();
        m_statusSelLabel->setText(QStringLiteral("已选：%1").arg(n));
        if (n > 1) {
            m_propertyPanel->setMultiSelection(n);
        } else {
            auto *item = selCtrl->primarySelected();
            if (item)
                m_propertyPanel->setShape(item->modelShape());
        }
        m_statusMessageLabel->setText(
            n == 1 ? QStringLiteral("已选择 1 个对象")
                   : QStringLiteral("已选择 %1 个对象").arg(n));
    }
}

void MainWindow::updateStatusBar()
{
    auto *doc = m_controller->document();
    int shapeCount = 0;
    for (const auto &layer : doc->layers())
        shapeCount += static_cast<int>(layer->shapes().size());
    m_statusCountLabel->setText(QStringLiteral("图层：%1  |  图形：%2")
                                    .arg(doc->layerCount())
                                    .arg(shapeCount));
}

void MainWindow::copySelection()
{
    auto *sel = m_canvasScene->selectionController();
    if (!sel || !sel->hasSelection()) return;

    QStringList ids;
    for (auto *item : sel->selectedItems()) {
        if (item && item->modelShape())
            ids.append(item->modelShape()->id());
    }
    if (!ids.isEmpty())
        m_controller->copyShapes(ids);
}

void MainWindow::cutSelection()
{
    auto *sel = m_canvasScene->selectionController();
    if (!sel || !sel->hasSelection()) return;

    QStringList ids;
    for (auto *item : sel->selectedItems()) {
        if (item && item->modelShape())
            ids.append(item->modelShape()->id());
    }
    if (ids.isEmpty()) return;

    m_controller->copyShapes(ids);
    for (const QString &id : ids)
        m_controller->deleteShape(id);
}

void MainWindow::pasteSelection()
{
    const QStringList newIds = m_controller->pasteShapes(20.0, 20.0);
    auto *sel = m_canvasScene->selectionController();
    if (sel && !newIds.isEmpty())
        sel->selectByShapeIds(newIds);
}

void MainWindow::deleteSelection()
{
    auto *sel = m_canvasScene->selectionController();
    if (!sel || !sel->hasSelection()) return;

    const auto selected = sel->selectedItems();
    QStringList ids;
    for (auto *item : selected) {
        if (item && item->modelShape())
            ids.append(item->modelShape()->id());
    }
    for (const QString &id : ids)
        m_controller->deleteShape(id);
}

void MainWindow::onCreateRect()
{
    setCurrentTool(QStringLiteral("矩形"));
    auto *rng = QRandomGenerator::global();
    m_controller->createRect(100 + rng->bounded(300), 100 + rng->bounded(200), 120, 80);
}

void MainWindow::onCreateEllipse()
{
    setCurrentTool(QStringLiteral("椭圆"));
    auto *rng = QRandomGenerator::global();
    m_controller->createEllipse(100 + rng->bounded(300), 100 + rng->bounded(200), 140, 100);
}

void MainWindow::onCreateTriangle()
{
    setCurrentTool(QStringLiteral("三角"));
    auto *rng = QRandomGenerator::global();
    m_controller->createTriangle(200 + rng->bounded(200), 200 + rng->bounded(200), 100);
}

void MainWindow::onCreatePentagon()
{
    setCurrentTool(QStringLiteral("五边"));
    auto *rng = QRandomGenerator::global();
    m_controller->createRegularPolygon(5, 200 + rng->bounded(200), 200 + rng->bounded(200), 70);
}

void MainWindow::onCreatePolyline()
{
    setCurrentTool(QStringLiteral("折线"));
    QVector<QPointF> pts = {{80, 300}, {150, 280}, {220, 320}, {300, 280}, {350, 330}};
    m_controller->createPolyline(pts);
}

void MainWindow::onCreateText()
{
    setCurrentTool(QStringLiteral("文本"));
    m_canvasView->setActiveTool(CanvasTool::Text);
    if (m_toolPalette)
        m_toolPalette->setCurrentTool(PaletteToolType::Text);
}

void MainWindow::onSelectTool()
{
    setCurrentTool(QStringLiteral("选择"));
    m_canvasView->setActiveTool(CanvasTool::Select);
    if (m_toolPalette)
        m_toolPalette->setCurrentTool(PaletteToolType::Select);
}

void MainWindow::onLineTool()
{
    setCurrentTool(QStringLiteral("直线"));
    m_canvasView->setActiveTool(CanvasTool::Line);
    if (m_toolPalette)
        m_toolPalette->setCurrentTool(PaletteToolType::Line);
}

void MainWindow::onToolPaletteSelected(PaletteToolType tool)
{
    switch (tool) {
    case PaletteToolType::Select:
        onSelectTool();
        break;
    case PaletteToolType::Line:
        onLineTool();
        break;
    case PaletteToolType::Rect:
        onCreateRect();
        break;
    case PaletteToolType::Ellipse:
        onCreateEllipse();
        break;
    case PaletteToolType::Triangle:
        onCreateTriangle();
        break;
    case PaletteToolType::Pentagon:
        onCreatePentagon();
        break;
    case PaletteToolType::Polyline:
        onCreatePolyline();
        break;
    case PaletteToolType::Text:
        onCreateText();
        break;
    }
}

void MainWindow::onNew()
{
    m_controller->undoStack()->clear();
    m_controller->document()->clear();
    m_canvasScene->syncAllFromDocument();
    m_filePath.clear();
    setWindowTitle(QStringLiteral("VectorGenEditor — 参数化矢量图形生成与编辑器"));
    m_statusMessageLabel->setText(QStringLiteral("已新建文档"));
}

void MainWindow::onOpen()
{
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("打开 JSON"),
        QString(), QStringLiteral("JSON 文件 (*.json);;所有文件 (*)"));
    if (path.isEmpty()) return;

    auto doc = JsonSerializer::loadFromFile(path);
    if (!doc) {
        QMessageBox::warning(this, QStringLiteral("打开失败"),
                             QStringLiteral("无法打开文件:\n%1").arg(path));
        return;
    }

    m_controller->undoStack()->clear();
    m_controller->document()->replaceFrom(*doc);
    m_canvasScene->syncAllFromDocument();
    m_filePath = path;
    setWindowTitle(QStringLiteral("VectorGenEditor — %1").arg(QFileInfo(path).fileName()));
    m_statusMessageLabel->setText(QStringLiteral("已打开 %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::onSave()
{
    if (m_filePath.isEmpty()) {
        onSaveAs();
        return;
    }
    if (!JsonSerializer::saveToFile(*m_controller->document(), m_filePath)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QStringLiteral("无法保存到:\n%1").arg(m_filePath));
    } else {
        m_statusMessageLabel->setText(QStringLiteral("已保存"));
    }
}

void MainWindow::onSaveAs()
{
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("另存为 JSON"),
        QString(), QStringLiteral("JSON 文件 (*.json);;所有文件 (*)"));
    if (path.isEmpty()) return;

    if (!JsonSerializer::saveToFile(*m_controller->document(), path)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QStringLiteral("无法保存到:\n%1").arg(path));
        return;
    }
    m_filePath = path;
    setWindowTitle(QStringLiteral("VectorGenEditor — %1").arg(QFileInfo(path).fileName()));
    m_statusMessageLabel->setText(QStringLiteral("已保存"));
}

void MainWindow::onExportSvg()
{
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出 SVG"),
        QString(), QStringLiteral("SVG 文件 (*.svg);;所有文件 (*)"));
    if (path.isEmpty()) return;

    if (!SvgExporter::exportToFile(*m_controller->document(), path)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                             QStringLiteral("无法导出 SVG:\n%1").arg(path));
    } else {
        m_statusMessageLabel->setText(QStringLiteral("SVG 已导出（范围：画板）"));
    }
}

void MainWindow::createMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("新建(&N)"), this, &MainWindow::onNew, QKeySequence::New);
    fileMenu->addAction(QStringLiteral("打开 JSON(&O)..."), this, &MainWindow::onOpen, QKeySequence::Open);
    fileMenu->addAction(QStringLiteral("保存 JSON(&S)"), this, &MainWindow::onSave, QKeySequence::Save);
    fileMenu->addAction(QStringLiteral("另存为(&A)..."), this, &MainWindow::onSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("导出 SVG(&E)..."), this, &MainWindow::onExportSvg);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, &QMainWindow::close, QKeySequence::Quit);

    QMenu *editMenu = menuBar()->addMenu(QStringLiteral("编辑(&E)"));
    editMenu->addAction(QStringLiteral("撤销(&U)"), m_controller->undoStack(), &QUndoStack::undo, QKeySequence::Undo);
    editMenu->addAction(QStringLiteral("重做(&R)"), m_controller->undoStack(), &QUndoStack::redo, QKeySequence::Redo);
    editMenu->addSeparator();
    editMenu->addAction(QStringLiteral("剪切(&T)"), this, [this]() { cutSelection(); }, QKeySequence::Cut);
    editMenu->addAction(QStringLiteral("复制(&C)"), this, [this]() { copySelection(); }, QKeySequence::Copy);
    editMenu->addAction(QStringLiteral("粘贴(&P)"), this, [this]() { pasteSelection(); }, QKeySequence::Paste);
    editMenu->addAction(QStringLiteral("删除(&D)"), this, [this]() { deleteSelection(); }, QKeySequence::Delete);
    editMenu->addAction(QStringLiteral("全选(&A)"), this, [this]() {
        auto *sel = m_canvasScene->selectionController();
        if (sel) sel->selectAll();
    }, QKeySequence::SelectAll);

    QMenu *viewMenu = menuBar()->addMenu(QStringLiteral("视图(&V)"));
    viewMenu->addAction(QStringLiteral("放大(&I)"), m_canvasView, &CanvasView::zoomIn);
    viewMenu->addAction(QStringLiteral("缩小(&O)"), m_canvasView, &CanvasView::zoomOut);
    viewMenu->addAction(QStringLiteral("适应画布(&F)"), m_canvasView, &CanvasView::fitWorkspaceDefault);
    viewMenu->addAction(QStringLiteral("适应窗口"), m_canvasView, &CanvasView::fitAll);
    m_gridAction = viewMenu->addAction(QStringLiteral("显示网格(&G)"));
    m_gridAction->setCheckable(true);
    m_gridAction->setChecked(m_canvasView->isGridVisible());
    connect(m_gridAction, &QAction::toggled, m_canvasView, &CanvasView::setGridVisible);

    QMenu *genMenu = menuBar()->addMenu(QStringLiteral("生成(&G)"));
    genMenu->addAction(QStringLiteral("生成星形"), m_generationPanel, &GenerationPanel::triggerGenerateStar);
    genMenu->addAction(QStringLiteral("生成箭头"), m_generationPanel, &GenerationPanel::triggerGenerateArrow);
    genMenu->addAction(QStringLiteral("生成阵列"), m_generationPanel, &GenerationPanel::triggerGenerateArray);

    QMenu *helpMenu = menuBar()->addMenu(QStringLiteral("帮助(&H)"));
    helpMenu->addAction(QStringLiteral("关于(&A)"), this, [this]() {
        QMessageBox::about(this, QStringLiteral("关于 VectorGenEditor"),
            QStringLiteral("VectorGenEditor\n参数化矢量图形生成与编辑系统\n\n"
                           "技术栈: C++17 / Qt5 Widgets / QGraphicsView\n"
                           "架构: Model - View - Command"));
    });
}

void MainWindow::createMainToolBar()
{
    QToolBar *bar = addToolBar(QStringLiteral("常用操作"));
    bar->setMovable(false);
    bar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    bar->setIconSize(QSize(18, 18));

    bar->addAction(QStringLiteral("↶ 撤销"), m_controller->undoStack(), &QUndoStack::undo);
    bar->addAction(QStringLiteral("↷ 重做"), m_controller->undoStack(), &QUndoStack::redo);
    bar->addSeparator();
    bar->addAction(QStringLiteral("⧉ 复制"), this, [this]() { copySelection(); });
    bar->addAction(QStringLiteral("📋 粘贴"), this, [this]() { pasteSelection(); });
    bar->addAction(QStringLiteral("🗑 删除"), this, [this]() { deleteSelection(); });
    bar->addSeparator();
    bar->addAction(QStringLiteral("💾 保存"), this, &MainWindow::onSave);
    bar->addAction(QStringLiteral("SVG 导出"), this, &MainWindow::onExportSvg);
}

QWidget *MainWindow::createCanvasArea()
{
    auto *host = new QWidget();
    host->setStyleSheet("background-color: #e8eaed;");

    auto *layout = new QVBoxLayout(host);
    layout->setContentsMargins(16, 16, 16, 16);

    auto *frame = new QFrame();
    frame->setObjectName("canvasFrame");
    auto *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    m_canvasView->setRenderHint(QPainter::Antialiasing);
    frameLayout->addWidget(m_canvasView);

    layout->addWidget(frame);
    return host;
}

QWidget *MainWindow::createRightPanel()
{
    auto *host = new QWidget();
    host->setObjectName("rightPanelHost");
    host->setMinimumWidth(360);

    auto *layout = new QVBoxLayout(host);
    layout->setContentsMargins(8, 8, 8, 8);

    m_rightTabs = new QTabWidget();
    m_rightTabs->addTab(m_propertyPanel, QStringLiteral("对象属性"));
    m_rightTabs->addTab(m_generationPanel, QStringLiteral("参数生成"));
    m_rightTabs->addTab(m_layerPanel, QStringLiteral("图层管理"));

    layout->addWidget(m_rightTabs);
    return host;
}

void MainWindow::createCentralLayout()
{
    auto *central = new QWidget();
    auto *row = new QHBoxLayout(central);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    auto *paletteHost = new QWidget(central);
    paletteHost->setObjectName(QStringLiteral("toolPaletteHost"));
    paletteHost->setFixedWidth(88);
    paletteHost->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto *paletteHostLayout = new QVBoxLayout(paletteHost);
    paletteHostLayout->setContentsMargins(0, 0, 0, 0);
    paletteHostLayout->setSpacing(0);

    m_toolPalette = new ToolPalette(paletteHost);
    connect(m_toolPalette, &ToolPalette::toolSelected,
            this, &MainWindow::onToolPaletteSelected);
    paletteHostLayout->addWidget(m_toolPalette);
    row->addWidget(paletteHost, 0);

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->addWidget(createCanvasArea());
    m_splitter->addWidget(createRightPanel());
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 0);
    m_splitter->setCollapsible(0, false);
    m_splitter->setCollapsible(1, false);
    m_splitter->setSizes({920, 380});
    row->addWidget(m_splitter, 1);

    setCentralWidget(central);
}

void MainWindow::createStatusBarWidgets()
{
    m_statusToolLabel = new QLabel(QStringLiteral("工具：选择"));
    m_statusPosLabel = new QLabel(QStringLiteral("坐标：0, 0"));
    m_statusZoomLabel = new QLabel(QStringLiteral("缩放：100%"));
    m_statusCountLabel = new QLabel(QStringLiteral("图层：1  |  图形：0"));
    m_statusSelLabel = new QLabel(QStringLiteral("已选：0"));
    m_statusMessageLabel = new QLabel(QStringLiteral("就绪"));

    statusBar()->addWidget(m_statusToolLabel, 1);
    statusBar()->addWidget(m_statusPosLabel, 1);
    statusBar()->addWidget(m_statusZoomLabel, 1);
    statusBar()->addWidget(m_statusCountLabel, 1);
    statusBar()->addWidget(m_statusSelLabel, 1);
    statusBar()->addPermanentWidget(m_statusMessageLabel, 2);
}
