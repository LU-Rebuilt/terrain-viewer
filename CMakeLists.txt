cmake_minimum_required(VERSION 3.25)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
project(terrain-viewer VERSION 0.1.0 LANGUAGES CXX DESCRIPTION "LEGO Universe terrain viewer")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets OpenGLWidgets REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)
FetchContent_Declare(lu_assets
    GIT_REPOSITORY https://github.com/LU-Rebuilt/lu-assets.git
    GIT_TAG main
)
FetchContent_MakeAvailable(lu_assets)

FetchContent_Declare(tool_common
    GIT_REPOSITORY https://github.com/LU-Rebuilt/tool-common.git
    GIT_TAG main
)
FetchContent_MakeAvailable(tool_common)

add_executable(terrain_viewer
    src/main.cpp
    src/main_window.cpp src/main_window.h
    src/terrain_gl_widget.cpp src/terrain_gl_widget.h
)
target_link_libraries(terrain_viewer PRIVATE gl_viewport lu_assets qt_common)
target_include_directories(terrain_viewer PRIVATE src)
