cmake_minimum_required(VERSION 3.16)

project(MemoPreview VERSION 0.1 LANGUAGES CXX)

# ============================================================================
# 项目配置
# ============================================================================
set(SOFTWARE_NAME "${PROJECT_NAME}")
configure_file("${CMAKE_SOURCE_DIR}/Config.h.in" "${CMAKE_BINARY_DIR}/Config.h")

# ============================================================================
# C++和Qt配置
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ============================================================================
# 依赖项
# ============================================================================
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Concurrent)

# Windows OLE库
add_library(WindowsOLE INTERFACE)
target_link_libraries(WindowsOLE INTERFACE
    oleacc
    ole32
    oleaut32
)

# ============================================================================
# 源文件组织
# ============================================================================

# UI层源文件
set(UI_SOURCES
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp
    src/ui/LogDialog.h
    src/ui/LogDialog.cpp
    src/ui/TemplateEditDialog.h
    src/ui/TemplateEditDialog.cpp
)

# 核心业务逻辑
set(CORE_SOURCES
    src/core/LatexmkManager.h
    src/core/LatexmkManager.cpp
    src/core/SuperMemoIeExtractor.h
    src/core/SuperMemoIeExtractor.cpp
    src/core/WindowEventMonitor.h
    src/core/WindowEventMonitor.cpp
)

# 工具类
set(UTILS_SOURCES
    src/utils/SuperMemoWindowInfo.h
    src/utils/SuperMemoWindowUtils.h
    src/utils/SuperMemoWindowUtils.cpp
    src/utils/SettingsUtils.h
    src/utils/SettingsUtils.cpp
)

# 主入口
set(MAIN_SOURCES
    src/main.cpp
)

# 所有源文件
set(PROJECT_SOURCES
    ${MAIN_SOURCES}
    ${UI_SOURCES}
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
)

# ============================================================================
# 可执行目标
# ============================================================================
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MemoPreview
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(MemoPreview SHARED ${PROJECT_SOURCES})
    else()
        add_executable(MemoPreview ${PROJECT_SOURCES})
    endif()
endif()

# ============================================================================
# 链接和包含目录
# ============================================================================
target_link_libraries(MemoPreview PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    WindowsOLE
)

# 添加include目录，使得可以使用 #include "ui/MainWindow.h" 等
target_include_directories(MemoPreview PRIVATE
    "${CMAKE_SOURCE_DIR}/src"           # 允许 #include "ui/xxx.h"
    "${CMAKE_SOURCE_DIR}/src/ui"        # 允许 #include "MainWindow.h" (UI内部)
    "${CMAKE_SOURCE_DIR}/src/core"      # 允许 #include "LatexmkManager.h" (core内部)
    "${CMAKE_SOURCE_DIR}/src/utils"     # 允许 #include "SuperMemoWindowUtils.h" (utils内部)
    "${CMAKE_BINARY_DIR}"               # 生成的Config.h
)

# ============================================================================
# 目标属性
# ============================================================================
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.MemoPreview)
endif()

set_target_properties(MemoPreview PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ============================================================================
# 安装规则
# ============================================================================
include(GNUInstallDirs)
install(TARGETS MemoPreview
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ============================================================================
# Qt6终结
# ============================================================================
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MemoPreview)
endif()
