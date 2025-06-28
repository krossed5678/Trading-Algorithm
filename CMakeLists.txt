cmake_minimum_required(VERSION 3.14)
project(trading_bot)

set(CMAKE_CXX_STANDARD 17)

# FetchContent for ImGui and GLFW
include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

# Your existing sources
set(SOURCES
    src/main.cpp
    src/DataLoader.cpp
    src/MovingAverage.cpp
    src/Strategy.cpp
    src/Backtester.cpp
    src/TradeLog.cpp
)

add_executable(trading_bot ${SOURCES})

# GUI sources
set(GUI_SOURCES
    src/gui_main.cpp
    src/DataLoader.cpp
    src/MovingAverage.cpp
    src/Strategy.cpp
    src/Backtester.cpp
    src/TradeLog.cpp
)

add_executable(trading_bot_gui ${GUI_SOURCES})

# ImGui sources (core + backends)
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add ImGui to GUI target
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

# Link everything for the GUI
find_package(OpenGL REQUIRED)
target_link_libraries(trading_bot_gui imgui glfw OpenGL::GL)
target_include_directories(trading_bot_gui PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

target_include_directories(trading_bot PRIVATE include)
target_include_directories(trading_bot_gui PRIVATE include)
