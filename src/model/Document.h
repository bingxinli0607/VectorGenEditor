#ifndef DOCUMENT_H
#define DOCUMENT_H

// ============================================================
// Document.h — 文档数据的核心头文件（只声明，不写具体逻辑）
//
// 【这个文件在多文件项目里扮演什么角色？】
//   .h 文件 = "说明书"：告诉编译器这个类有哪些成员、长什么样
//   .cpp 文件 = "实现"：具体代码写在 Document.cpp 里
//
// 【它和谁连接？】
//   main.cpp              → 创建 Document
//   DocumentController    → 通过命令修改 Document（增删改图形）
//   CanvasScene           → 监听 Document 的信号，刷新画布显示
//   JsonSerializer        → 调用 toJson() / fromJson() 保存/读取项目
//
// 【数据层级】
//   Document（整份文档）
//     └── Layer（图层，可多个）
//           └── Shape（图形：矩形、椭圆、文本…）
// ============================================================

#include <QObject>      // Qt 基类，提供 signals/slots 机制
#include <QJsonObject>  // JSON 对象，用于保存/读取 .json 项目文件
#include <QRectF>         // 浮点矩形 (x, y, width, height)，表示画板范围
#include <vector>         // C++ 标准库：动态数组，类似 QList
#include <memory>         // 提供 std::unique_ptr（智能指针，自动管理内存）

// --- 前向声明（Forward Declaration）---
// 这里只写 "class 名字;"，不 #include 完整头文件。
// 好处：.h 文件更轻量，避免头文件互相循环引用。
// 真正用到 Layer/Shape 的细节时，在 Document.cpp 里 #include 即可。
class Layer;
class Shape;

/// Document：所有矢量数据的唯一来源（Single Source of Truth）
/// 模型层只存数据，不依赖 QGraphicsScene（界面和 data 分离）
class Document : public QObject
{
    Q_OBJECT  // 宏：启用 Qt 信号/槽；没有它下面的 signals: 无法使用

public:
    // explicit：禁止隐式类型转换，避免误写 Document d = parent; 这类代码
    // QObject *parent：Qt 对象树父节点；main.cpp 里传 &app，程序退出时自动释放
    explicit Document(QObject *parent = nullptr);
    ~Document() override;  // override：明确这是重写父类虚析构函数

    // ==================== 图层相关 ====================
    // const 在末尾 = "只读访问"，不会修改 Document 内部数据
    // & 表示返回引用（不拷贝整个 vector，效率更高）
    // std::unique_ptr<Layer> = 独占所有权指针，Document 销毁时 Layer 自动释放
    const std::vector<std::unique_ptr<Layer>> &layers() const;
    std::vector<std::unique_ptr<Layer>> &layers();  // 非 const 版本，允许修改
    Layer *currentLayer() const;       // 返回当前正在编辑的图层（裸指针，不拥有所有权）
    int currentLayerIndex() const;     // 当前图层在 m_layers 里的下标
    void setCurrentLayer(int index);   // 切换当前图层 → 会 emit layerChanged()

    Layer *addLayer(const QString &name);  // 新建图层
    void removeLayer(int index);           // 删除图层（至少保留 1 个）
    Layer *layerAt(int index) const;       // 按下标取图层，越界返回 nullptr
    int layerCount() const;

    // ==================== 图形操作（默认作用在当前图层） ====================
    // std::unique_ptr<Shape>：调用方把 Shape 的所有权"交"给 Document
    // Document 收到后放进 Layer，之后由 Document/Layer 负责释放
    void addShape(std::unique_ptr<Shape> shape);
    // takeShape：从文档中"取走"图形（所有权转移给调用方），用于删除/剪切
    std::unique_ptr<Shape> takeShape(const QString &shapeId);
    // findShape：只查找，不转移所有权；找不到返回 nullptr
    Shape *findShape(const QString &shapeId) const;

    // 调整图形在同一图层内的叠放顺序（z-order，谁在上谁在下）
    void moveShapeUp(const QString &shapeId);
    void moveShapeDown(const QString &shapeId);
    void moveShapeToTop(const QString &shapeId);
    void moveShapeToBottom(const QString &shapeId);

    // ==================== 导入 / 导出 ====================
    QJsonObject toJson() const;  // 把整个文档序列化成 JSON
    // static：不依赖某个 Document 实例，直接 Document::fromJson(...) 调用
    static std::unique_ptr<Document> fromJson(const QJsonObject &obj, QObject *parent = nullptr);

    // ==================== 画板（白色区域） ====================
    // pageRect / workspaceRect 目前指向同一块数据 m_workspaceRect
    // 表示白色画板的范围；导出 SVG 时以它为边界
    QRectF pageRect() const { return m_workspaceRect; }
    void setPageRect(const QRectF &rect) { m_workspaceRect = rect; }
    QRectF workspaceRect() const { return m_workspaceRect; }
    void setWorkspaceRect(const QRectF &rect) { m_workspaceRect = rect; }

    // ==================== 清空 / 替换 ====================
    void clear();                              // 清空所有图层和图形
    void replaceFrom(const Document &other);   // 用另一份文档的内容覆盖当前文档

signals:
    // --- Qt 信号：数据变了就"广播"，谁关心谁用 connect 监听 ---
    // 典型连接（在 CanvasScene 等处）：
    //   connect(document, &Document::shapeAdded,   scene, &CanvasScene::onShapeAdded);
    //   connect(document, &Document::shapeChanged, scene, &CanvasScene::onShapeChanged);
    void documentChanged();                  // 文档整体有变化（如清空、批量替换）
    void shapeAdded(const QString &shapeId);   // 新增了一个图形
    void shapeRemoved(const QString &shapeId); // 删除了一个图形
    void shapeChanged(const QString &shapeId); // 某个图形属性/几何被修改
    void layerChanged();                     // 图层增删或当前图层切换

private:
    std::vector<std::unique_ptr<Layer>> m_layers;  // 所有图层；m_ 前缀 = 成员变量
    int m_currentLayerIndex = 0;                   // 当前图层下标，默认第 0 层
    QRectF m_workspaceRect;                        // 白色画板矩形
};

#endif // DOCUMENT_H
