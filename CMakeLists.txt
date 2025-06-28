cmake_minimum_required(VERSION 3.14)
project(trading_bot CXX)

# Check if CUDA is available
find_package(CUDA QUIET)
if(CUDA_FOUND)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_ARCHITECTURES native)
    set(USE_CUDA TRUE)
    message(STATUS "CUDA found - GPU acceleration enabled")
else()
    set(USE_CUDA FALSE)
    message(STATUS "CUDA not found - using CPU-only version")
endif()

set(CMAKE_CXX_STANDARD 17)

# Enable maximum CPU optimizations
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")

# Enable SIMD optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx2 -msse4.2 -mfma")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -mavx2 -msse4.2 -mfma")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /arch:AVX2")
endif()

# CUDA optimizations
if(USE_CUDA)
    set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} -O3 -arch=sm_60")
    set(CMAKE_CUDA_FLAGS_DEBUG "${CMAKE_CUDA_FLAGS_DEBUG} -O0 -g -G")
    
    # Enable CUDA optimizations
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --use_fast_math")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --maxrregcount=32")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --ptxas-options=-v")
endif()

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

# CUDA sources (only if CUDA is available)
if(USE_CUDA)
    set(CUDA_SOURCES
        src/gpu_kernels.cu
        src/GPUStrategy.cpp
    )
else()
    set(CUDA_SOURCES "")
endif()

add_executable(trading_bot ${SOURCES} ${CUDA_SOURCES})

# Benchmark executable
add_executable(benchmark_performance benchmark_performance.cpp ${SOURCES} ${CUDA_SOURCES})

# GUI sources
set(GUI_SOURCES
    src/gui_main.cpp
    src/DataLoader.cpp
    src/MovingAverage.cpp
    src/Strategy.cpp
    src/Backtester.cpp
    src/TradeLog.cpp
    ${CUDA_SOURCES}
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

# Add ImGui to GUI target with proper includes
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
    ${glfw_SOURCE_DIR}/include
)

# Link everything for the GUI
find_package(OpenGL REQUIRED)
target_link_libraries(trading_bot_gui imgui glfw OpenGL::GL)
target_include_directories(trading_bot_gui PRIVATE 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
    ${glfw_SOURCE_DIR}/include
)

target_include_directories(trading_bot PRIVATE include)
target_include_directories(trading_bot_gui PRIVATE include)
target_include_directories(benchmark_performance PRIVATE include)

# Add compile definitions
if(USE_CUDA)
    target_compile_definitions(trading_bot PRIVATE USE_CUDA=1)
    target_compile_definitions(trading_bot_gui PRIVATE USE_CUDA=1)
    target_compile_definitions(benchmark_performance PRIVATE USE_CUDA=1)
else()
    target_compile_definitions(trading_bot PRIVATE USE_CUDA=0)
    target_compile_definitions(trading_bot_gui PRIVATE USE_CUDA=0)
    target_compile_definitions(benchmark_performance PRIVATE USE_CUDA=0)
endif()

# Enable link-time optimization for release builds
set_property(TARGET trading_bot PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set_property(TARGET trading_bot_gui PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set_property(TARGET benchmark_performance PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

# Set optimization flags for specific targets
target_compile_options(trading_bot PRIVATE $<$<CONFIG:Release>:-O3>)
target_compile_options(trading_bot_gui PRIVATE $<$<CONFIG:Release>:-O3>)
target_compile_options(benchmark_performance PRIVATE $<$<CONFIG:Release>:-O3>)

# Enable CUDA optimizations for CUDA targets
if(USE_CUDA)
    set_target_properties(trading_bot PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_ARCHITECTURES native
    )
    set_target_properties(trading_bot_gui PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_ARCHITECTURES native
    )
    set_target_properties(benchmark_performance PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_ARCHITECTURES native
    )
endif()

# Print optimization status
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
if(USE_CUDA)
    message(STATUS "CUDA flags: ${CMAKE_CUDA_FLAGS}")
endif()
