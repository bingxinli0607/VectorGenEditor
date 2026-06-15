#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ToolPalette.h"

class QAction;
class QKeyEvent;
class QSplitter;
class QTabWidget;
class QLabel;
class CanvasView;
class CanvasScene;
class DocumentController;
class PropertyPanel;
class GenerationPanel;
class LayerPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(DocumentController *controller,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

    CanvasView  *canvasView()  const { return m_canvasView; }
    CanvasScene *canvasScene() const { return m_canvasScene; }
    PropertyPanel *propertyPanel() const { return m_propertyPanel; }

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onExportSvg();
    void onCreateRect();
    void onCreateEllipse();
    void onCreateTriangle();
    void onCreatePentagon();
    void onCreatePolyline();
    void onCreateText();
    void onSelectTool();
    void onLineTool();
    void onToolPaletteSelected(PaletteToolType tool);
    void onSelectionChanged();
    void updateStatusBar();
    void setCurrentTool(const QString &toolName);

private:
    void applyStyleSheet();
    void createMenuBar();
    void createMainToolBar();
    void createStatusBarWidgets();
    void createCentralLayout();
    QWidget *createCanvasArea();
    QWidget *createRightPanel();

    void copySelection();
    void cutSelection();
    void pasteSelection();
    void deleteSelection();

    DocumentController  *m_controller      = nullptr;
    CanvasView          *m_canvasView        = nullptr;
    CanvasScene         *m_canvasScene       = nullptr;
    PropertyPanel       *m_propertyPanel     = nullptr;
    GenerationPanel     *m_generationPanel = nullptr;
    LayerPanel          *m_layerPanel        = nullptr;
    QTabWidget          *m_rightTabs         = nullptr;
    QSplitter           *m_splitter          = nullptr;
    ToolPalette         *m_toolPalette       = nullptr;

    QLabel *m_statusToolLabel   = nullptr;
    QLabel *m_statusPosLabel    = nullptr;
    QLabel *m_statusZoomLabel   = nullptr;
    QLabel *m_statusCountLabel  = nullptr;
    QLabel *m_statusSelLabel    = nullptr;
    QLabel *m_statusMessageLabel = nullptr;

    QAction *m_gridAction = nullptr;
    QString  m_filePath;
    QString  m_currentToolName = QStringLiteral("选择");
};

#endif // MAINWINDOW_H
