cmake_minimum_required(VERSION 3.26)
project(path_planner_gui)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL 3.2 REQUIRED)

set(CMAKE_CXX_STANDARD 17)

set(IMGUI_SOURCES
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp
        imgui/backends/imgui_impl_opengl3.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/misc/cpp/imgui_stdlib.cpp
        ImGuiFileDialog/ImGuiFileDialog.cpp
        implot/implot.h
        implot/implot.cpp
        implot/implot_internal.h
        implot/implot_items.cpp
)

add_executable(path_planner_gui main.cpp ${IMGUI_SOURCES})

target_link_libraries(path_planner_gui PRIVATE glfw OpenGL::GL)
target_include_directories(path_planner_gui PRIVATE imgui ImGuiFileDialog)
