cmake_minimum_required(VERSION 3.16)

project(MemoPreview VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Concurrent)

# 创建导入库目标
add_library(WindowsOLE INTERFACE)
target_link_libraries(WindowsOLE INTERFACE
    oleacc
    ole32
    oleaut32
)

set(PROJECT_SOURCES
        main.cpp
        MainWindow.cpp
        MainWindow.h
        SettingsDialog.h SettingsDialog.cpp
        TemplateEditDialog.h TemplateEditDialog.cpp
        LatexmkManager.h LatexmkManager.cpp
        LogDialog.h LogDialog.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MemoPreview
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        SuperMemoWindowInfo.h
        SuperMemoWindowUtils.h SuperMemoWindowUtils.cpp
        SuperMemoIeExtractor.h SuperMemoIeExtractor.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET MemoPreview APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(MemoPreview SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(MemoPreview
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(MemoPreview PRIVATE Qt${QT_VERSION_MAJOR}::Widgets WindowsOLE Qt6::Concurrent)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
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

include(GNUInstallDirs)
install(TARGETS MemoPreview
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MemoPreview)
endif()
