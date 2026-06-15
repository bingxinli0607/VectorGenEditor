# VectorGenEditor

参数化矢量图形生成与编辑器，基于 C++17、Qt 5.15.2 Widgets 和 QGraphicsView / QGraphicsScene 实现。

## 项目简介

VectorGenEditor 的目标是把“参数生成”和“直接编辑”放在同一个桌面端工作流里。当前代码采用 Model-View-Command 风格的 MVC 变体，`Document` 是唯一数据源，所有可撤销的写操作都通过 `QUndoStack` 进入命令栈。

### 当前已经具备的能力

- 图形创建：矩形、椭圆、三角形、五边形、多边形、折线、直线、文本、星形、箭头
- 选择与编辑：单选、多选、框选、拖拽移动、属性修改
- 图层管理：多图层、可见性切换、层级顺序管理
- 画板逻辑：白色画板 + 灰色无限画布，导出时以画板范围为主
- 文件导出：JSON 项目保存 / 读取，结构化 SVG 导出
- 撤销重做：通过 `QUndoCommand` + `QUndoStack` 统一管理

## 界面结构

主界面由三栏和底部状态栏组成：

| 区域 | 内容 |
|------|------|
| 顶部 | 菜单栏 + 常用工具栏（撤销、重做、复制、粘贴、删除、保存、导出 SVG） |
| 左侧 | 工具栏（选择、直线、矩形、椭圆、三角形、五边形、折线、文本等） |
| 中间 | `CanvasView` / `CanvasScene` 画布工作区，包含网格、白色画板和可滚动背景 |
| 右侧 | 属性区：对象属性、参数生成、图层管理 |
| 底部 | 状态栏：当前工具、坐标、缩放比例、选择数量、提示信息 |

中央区域通过 `QSplitter` 组织，便于调整左中右三栏宽度。

## 架构说明

项目的真实数据流可以理解为：

```text
用户操作 → View 事件 → Controller → QUndoCommand → Document
Document 变化 → signals → CanvasScene / 右侧面板同步更新
```

### 核心原则

1. `Document` 是唯一数据源。
2. `QGraphicsScene` 只负责显示和交互，不保存业务真相。
3. 所有重要写操作尽量走命令，保证撤销 / 重做可用。
4. SVG 导出采用结构化元素输出，不使用截图式渲染。

## 主要目录

```text
src/
├── main.cpp
├── app/              应用封装
├── model/            Document、Layer、Shape、Geometry、Workspace
├── view/             MainWindow、CanvasView、CanvasScene、面板与工具栏
├── controller/       DocumentController、SelectionController
├── command/          撤销 / 重做命令
└── serialization/    JSON 与 SVG 导出
```

### 你需要优先认识的类

- [src/main.cpp](src/main.cpp)：程序入口，理解对象是怎么串起来的
- [src/model/Document.h](src/model/Document.h)：文档数据核心
- [src/model/Shape.h](src/model/Shape.h)：图形基类，理解图形数据长什么样
- [src/controller/DocumentController.h](src/controller/DocumentController.h)：创建、删除、修改的总入口
- [src/controller/SelectionController.h](src/controller/SelectionController.h)：选中、多选、框选、手柄逻辑
- [src/view/CanvasView.h](src/view/CanvasView.h)：鼠标、滚轮、键盘等交互入口
- [src/view/CanvasScene.h](src/view/CanvasScene.h)：把模型同步成可视化 item
- [src/serialization/SvgExporter.h](src/serialization/SvgExporter.h)：SVG 是怎么导出的

## 画板与导出

当前项目把“画板”和“无限画布”分开处理：

- `Document::workspaceRect()` 保存白色画板范围
- `Artboard::rect()` 提供统一的画板矩形入口
- `Workspace` 负责工作区边界、裁剪和约束判断
- `SvgExporter` 以画板矩形作为 `viewBox`，并使用裁剪逻辑限制导出范围

这意味着你可以在灰色区域继续放草稿对象，但导出默认以白色画板为主。

## 学习顺序建议

如果你是新手，建议按这个顺序看代码：

1. 先看 [src/main.cpp](src/main.cpp)，弄清楚应用启动流程
2. 再看 [src/model/Document.h](src/model/Document.h)，理解数据存放位置
3. 然后看 [src/model/Shape.h](src/model/Shape.h) 和几个具体图形类，理解形状如何表示
4. 接着看 [src/controller/DocumentController.h](src/controller/DocumentController.h)，理解操作如何进入模型
5. 再看 [src/controller/SelectionController.h](src/controller/SelectionController.h)，理解选择系统
6. 然后看 [src/view/CanvasView.h](src/view/CanvasView.h) 和 [src/view/CanvasScene.h](src/view/CanvasScene.h)，理解交互和同步
7. 最后看 [src/serialization/SvgExporter.h](src/serialization/SvgExporter.h)，理解导出逻辑

## 构建与运行

### Windows / Visual Studio 2022

```powershell
cd D:\Users\20371\Desktop\VectorGenEditor
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
New-Item -ItemType Directory build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

运行：

```powershell
.\build\Release\VectorGenEditor.exe
```

### Linux / macOS

```bash
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
```

## 测试

项目根目录下有多个阶段性测试目标，构建后可在 `build` 目录中运行对应可执行文件。

## 相关文档

- [docs/architecture.md](docs/architecture.md)：当前架构说明
- [docs/demo-steps.md](docs/demo-steps.md)：演示步骤与界面说明

## 适合记住的一句话

如果你在这个项目里只记住一件事，那就是：**数据在 Document，界面只是显示，所有修改尽量通过控制器和命令完成。**
