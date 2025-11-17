# MemoPreview 项目结构

本文档说明了项目的目录组织和架构设计。

## 📁 目录结构

```
MemoPreview/
├── CMakeLists.txt          # 主CMake构建配置
├── Config.h.in             # 配置文件模板
├── README.md               # 项目说明（英文）
├── README_zh.md            # 项目说明（中文）
├── .gitignore              # Git忽略规则
│
└── src/                    # 源代码目录
    ├── main.cpp            # 程序入口点
    │
    ├── ui/                 # 用户界面层
    │   ├── MainWindow.h/cpp            # 主窗口
    │   ├── SettingsDialog.h/cpp        # 设置对话框
    │   ├── LogDialog.h/cpp             # 日志窗口
    │   └── TemplateEditDialog.h/cpp    # 模板编辑对话框
    │
    ├── core/               # 核心业务逻辑层
    │   ├── LatexmkManager.h/cpp        # LaTeX编译管理
    │   ├── SuperMemoIeExtractor.h/cpp  # SuperMemo内容提取
    │   └── WindowEventMonitor.h/cpp    # Windows事件监控器
    │
    └── utils/              # 工具类层
        ├── SuperMemoWindowInfo.h       # 窗口信息结构
        ├── SuperMemoWindowUtils.h/cpp  # SuperMemo窗口工具
        └── SettingsUtils.h/cpp         # 设置工具
```

## 🏗️ 架构分层

### 1. UI层 (`src/ui/`)
**职责**：用户界面和交互逻辑
- `MainWindow`: 主窗口，协调各模块
- `SettingsDialog`: 应用设置界面
- `LogDialog`: 日志显示窗口
- `TemplateEditDialog`: LaTeX模板编辑

**特点**：
- 依赖Qt Widgets框架
- 可调用core层和utils层
- 不包含复杂业务逻辑

### 2. Core层 (`src/core/`)
**职责**：核心业务逻辑和功能实现
- `LatexmkManager`: 管理latexmk进程，监控编译状态
- `SuperMemoIeExtractor`: 从IE控件提取HTML内容，DOM遍历
- `WindowEventMonitor`: Windows事件钩子，事件驱动机制

**特点**：
- 独立的业务单元
- 可被UI层或其他层调用
- 包含复杂的算法和逻辑

### 3. Utils层 (`src/utils/`)
**职责**：通用工具和辅助功能
- `SuperMemoWindowUtils`: 枚举SuperMemo窗口
- `SuperMemoWindowInfo`: 窗口信息数据结构
- `SettingsUtils`: 设置初始化和管理

**特点**：
- 无状态或轻量级
- 可被任何层调用
- 通用性强，可复用

## 🔄 依赖关系

```
┌─────────┐
│   UI    │  ← 用户界面层
└────┬────┘
     │
     ↓ 调用
┌─────────┐
│  Core   │  ← 核心业务层
└────┬────┘
     │
     ↓ 调用
┌─────────┐
│ Utils   │  ← 工具层
└─────────┘
```

**依赖原则**：
- UI层可以依赖Core层和Utils层
- Core层可以依赖Utils层
- Utils层不依赖其他层（最底层）
- 同层之间尽量避免依赖

## 📝 CMake组织

CMakeLists.txt按功能模块组织源文件：

```cmake
# UI层源文件
set(UI_SOURCES
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    ...
)

# 核心业务逻辑
set(CORE_SOURCES
    src/core/LatexmkManager.h
    src/core/LatexmkManager.cpp
    ...
)

# 工具类
set(UTILS_SOURCES
    src/utils/SuperMemoWindowUtils.h
    src/utils/SuperMemoWindowUtils.cpp
    ...
)
```

**Include路径配置**：
```cmake
target_include_directories(MemoPreview PRIVATE
    "${CMAKE_SOURCE_DIR}/src"        # 跨层调用: #include "core/xxx.h"
    "${CMAKE_SOURCE_DIR}/src/ui"     # UI内部: #include "MainWindow.h"
    "${CMAKE_SOURCE_DIR}/src/core"   # Core内部: #include "LatexmkManager.h"
    "${CMAKE_SOURCE_DIR}/src/utils"  # Utils内部: #include "SettingsUtils.h"
    "${CMAKE_BINARY_DIR}"            # 生成文件: #include "Config.h"
)
```

## 🎯 设计原则

本项目遵循以下现代C++/CMake最佳实践：

### 1. **关注点分离**
- UI与业务逻辑分离
- 业务逻辑与工具函数分离
- 每个类职责单一

### 2. **清晰的目录结构**
- 按功能而非文件类型组织
- 三层架构：UI → Core → Utils
- 便于导航和维护

### 3. **模块化**
- 每个模块职责明确
- 低耦合，高内聚
- 便于单元测试

### 4. **可扩展性**
- 易于添加新的UI组件（放入`src/ui/`）
- 易于添加新的核心功能（放入`src/core/`）
- 易于添加新的工具类（放入`src/utils/`）

## 🚀 构建说明

### 标准构建流程：
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
cmake --build .

# 安装（可选）
cmake --install .
```

### IDE支持：
- **Qt Creator**: 直接打开CMakeLists.txt
- **Visual Studio**: 打开文件夹或生成.sln
- **CLion**: 直接打开项目目录

## 📚 代码组织规范

### 头文件包含顺序：
1. 对应的.h文件（如果是.cpp）
2. Qt框架头文件
3. 标准库头文件
4. 本项目头文件（按层次：utils → core → ui）

### 示例：
```cpp
#include "MainWindow.h"           // 1. 对应头文件

#include <QToolBar>               // 2. Qt框架
#include <QLabel>

#include <vector>                 // 3. 标准库
#include <memory>

#include "core/LatexmkManager.h"  // 4. 本项目（按层次）
#include "utils/SettingsUtils.h"
```

## 🔧 维护指南

### 添加新功能时：
1. **UI功能**：在`src/ui/`创建新的对话框或窗口类
2. **业务逻辑**：在`src/core/`创建新的管理器或处理器
3. **工具函数**：在`src/utils/`创建新的工具类

### 更新CMakeLists.txt：
在对应的`XXX_SOURCES`变量中添加新文件：
```cmake
set(CORE_SOURCES
    ...
    src/core/NewFeature.h
    src/core/NewFeature.cpp
)
```

## 📊 项目统计

- **总文件数**: 20个源文件（.h/.cpp）
- **代码行数**: 约1800行
- **模块数量**: 3层（UI/Core/Utils）
- **主要依赖**: Qt6, Windows API (OLE/MSHTML)

---

*最后更新: 2025-11-17*
*遵循: 现代C++17标准 + CMake 3.16+*
