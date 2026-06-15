cmake_minimum_required(VERSION 3.10)
project(VectorGenEditor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- Platform-specific settings ---
if(MSVC)
    # Avoid C4819 warnings for UTF-8 source files
    add_compile_options(/utf-8)
else()
    # Linux/conda: Qt5 installed via anaconda3
    set(CMAKE_PREFIX_PATH "/mnt/sda/libingxin/anaconda3/lib/cmake" ${CMAKE_PREFIX_PATH})
endif()

# --- Find Qt5 ---
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

message(STATUS "Qt5 version: ${Qt5_VERSION}")
message(STATUS "Qt5 include: ${Qt5Core_INCLUDE_DIRS}")

# --- Sources ---
set(SOURCES
    # main
    src/main.cpp

    # app
    src/app/Application.cpp

    # model
    src/model/Shape.cpp
    src/model/Geometry.cpp
    src/model/Workspace.cpp
    src/model/LineShape.cpp
    src/model/RectShape.cpp
    src/model/EllipseShape.cpp
    src/model/PolygonShape.cpp
    src/model/PolylineShape.cpp
    src/model/TextShape.cpp
    src/model/StarShape.cpp
    src/model/ArrowShape.cpp
    src/model/Layer.cpp
    src/model/Document.cpp
    src/model/ShapeFactory.cpp

    # serialization
    src/serialization/JsonSerializer.cpp
    src/serialization/SvgExporter.cpp

    # command
    src/command/AddShapeCommand.cpp
    src/command/RemoveShapeCommand.cpp
    src/command/MoveShapeCommand.cpp
    src/command/MoveShapesCommand.cpp
    src/command/ResizeShapeCommand.cpp
    src/command/ChangeStyleCommand.cpp
    src/command/ChangeTextCommand.cpp
    src/command/RotateShapeCommand.cpp
    src/command/ModifyShapeCommand.cpp

    # controller
    src/controller/DocumentController.cpp
    src/controller/SelectionController.cpp

    # view
    src/view/MainWindow.cpp
    src/view/CanvasView.cpp
    src/view/CanvasScene.cpp
    src/view/ShapeGraphicsItem.cpp
    src/view/HandleItem.cpp
    src/view/PropertyPanel.cpp
    src/view/LayerPanel.cpp
    src/view/GenerationPanel.cpp
    src/view/ToolPalette.cpp
)

set(HEADERS
    # app
    src/app/Application.h

    # model
    src/model/Shape.h
    src/model/Geometry.h
    src/model/Workspace.h
    src/model/Artboard.h
    src/model/LineShape.h
    src/model/RectShape.h
    src/model/EllipseShape.h
    src/model/PolygonShape.h
    src/model/PolylineShape.h
    src/model/TextShape.h
    src/model/StarShape.h
    src/model/ArrowShape.h
    src/model/Layer.h
    src/model/Document.h
    src/model/ShapeFactory.h

    # serialization
    src/serialization/JsonSerializer.h
    src/serialization/SvgExporter.h

    # command
    src/command/AddShapeCommand.h
    src/command/RemoveShapeCommand.h
    src/command/MoveShapeCommand.h
    src/command/MoveShapesCommand.h
    src/command/ResizeShapeCommand.h
    src/command/ChangeStyleCommand.h
    src/command/ChangeTextCommand.h
    src/command/RotateShapeCommand.h
    src/command/ModifyShapeCommand.h

    # controller
    src/controller/DocumentController.h
    src/controller/SelectionController.h

    # view
    src/view/MainWindow.h
    src/view/CanvasView.h
    src/view/CanvasScene.h
    src/view/ShapeGraphicsItem.h
    src/view/HandleItem.h
    src/view/PropertyPanel.h
    src/view/LayerPanel.h
    src/view/GenerationPanel.h
    src/view/ToolPalette.h
    src/view/UiStyles.h
)

# ===================================================================
# Helper: apply platform-specific linker settings to a target.
# On Linux/conda we need conda's libstdc++ (system libstdc++ lacks
# CXXABI_1.3.15) and RPATH so the binary finds conda libs at runtime.
# On MSVC none of this applies — MSVC links its own runtime.
# ===================================================================
function(apply_platform_linker_settings TARGET_NAME)
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
    )
    if(NOT MSVC)
        target_link_directories(${TARGET_NAME} PRIVATE
            /mnt/sda/libingxin/anaconda3/lib
        )
        target_link_libraries(${TARGET_NAME} PRIVATE stdc++)
        set_target_properties(${TARGET_NAME} PROPERTIES
            INSTALL_RPATH "/mnt/sda/libingxin/anaconda3/lib"
            BUILD_RPATH   "/mnt/sda/libingxin/anaconda3/lib"
        )
    endif()
endfunction()

# --- Executable ---
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
apply_platform_linker_settings(${PROJECT_NAME})

# --- Self-Test (exclude main.cpp to avoid duplicate main) ---
set(TEST_SOURCES ${SOURCES})
list(REMOVE_ITEM TEST_SOURCES "src/main.cpp")

add_executable(test_stage3
    test/test_stage3.cpp
    ${TEST_SOURCES}
    ${HEADERS}
)
apply_platform_linker_settings(test_stage3)

# --- Stage 4 Test ---
add_executable(test_stage4
    test/test_stage4.cpp
    ${TEST_SOURCES}
    ${HEADERS}
)
apply_platform_linker_settings(test_stage4)

# --- Stage 5 Test ---
add_executable(test_stage5
    test/test_stage5.cpp
    ${TEST_SOURCES}
    ${HEADERS}
)
apply_platform_linker_settings(test_stage5)

# --- Stage 7 Test ---
add_executable(test_stage7
    test/test_stage7.cpp
    ${TEST_SOURCES}
    ${HEADERS}
)
apply_platform_linker_settings(test_stage7)

# --- Stage 8 Test ---
add_executable(test_stage8
    test/test_stage8.cpp
    ${TEST_SOURCES}
    ${HEADERS}
)
apply_platform_linker_settings(test_stage8)

# --- Stage 9 Test (selection / multi-move / workspace) ---
add_executable(test_stage9
    test/test_stage9.cpp
    ${TEST_SOURCES}
    ${HEADERS}
)
apply_platform_linker_settings(test_stage9)

# --- Stage 10 Test (resize / rotate) ---
add_executable(test_stage10
    test/test_stage10.cpp
    ${TEST_SOURCES}
    ${HEADERS}
)
apply_platform_linker_settings(test_stage10)

# --- Demo Generator ---
add_executable(generate_demo
    test/generate_demo.cpp
    ${TEST_SOURCES}
    ${HEADERS}
)
apply_platform_linker_settings(generate_demo)

# --- Install ---
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
