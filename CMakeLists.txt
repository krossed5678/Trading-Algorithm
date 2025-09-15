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
    message(STATUS "CUDA found: ${CUDA_TOOLKIT_ROOT_DIR}")
    add_definitions(-DUSE_CUDA)
    set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -O3")
    cuda_add_library(gpu_kernels src/gpu_kernels.cu)
    set(GPU_KERNELS_LIB gpu_kernels)
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
    src/strategy_grid_search.cpp
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

# Core sources (no main functions)
set(CORE_SOURCES
    src/Backtester.cpp
    src/DataLoader.cpp
    src/GeneticStrategy.cpp
    src/GPUStrategy.cpp
    src/MovingAverage.cpp
    src/Strategy.cpp
    src/main.cpp
    src/genetic_evolution.cpp
    src/strategy_grid_search.cpp
)

add_library(trading_core STATIC ${CORE_SOURCES})
target_include_directories(trading_core PUBLIC include)
target_link_libraries(trading_core PRIVATE ${GPU_KERNELS_LIB})

# Main CLI executable
add_executable(trading_bot src/main.cpp)
target_link_libraries(trading_bot PRIVATE trading_core)
target_include_directories(trading_bot PRIVATE include)

# Benchmark executable
add_executable(benchmark_performance benchmark_performance.cpp)
target_link_libraries(benchmark_performance PRIVATE trading_core)
target_include_directories(benchmark_performance PRIVATE include)

# GUI executable
add_executable(trading_bot_gui src/gui_main.cpp)
target_link_libraries(trading_bot_gui PRIVATE trading_core imgui glfw OpenGL::GL)
target_include_directories(trading_bot_gui PRIVATE include ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${glfw_SOURCE_DIR}/include)

# Grid search executable
add_executable(strategy_grid_search src/strategy_grid_search.cpp)
target_link_libraries(strategy_grid_search PRIVATE trading_core)
target_include_directories(strategy_grid_search PRIVATE include)

# Genetic evolution executable
add_executable(genetic_evolution src/genetic_evolution.cpp)
target_link_libraries(genetic_evolution PRIVATE trading_core ${GPU_KERNELS_LIB})
target_include_directories(genetic_evolution PRIVATE include)
if(USE_CUDA)
    target_compile_definitions(genetic_evolution PRIVATE USE_CUDA=1)
else()
    target_compile_definitions(genetic_evolution PRIVATE USE_CUDA=0)
endif()
set_property(TARGET genetic_evolution PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
target_compile_options(genetic_evolution PRIVATE $<$<CONFIG:Release>:-O3>)

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
