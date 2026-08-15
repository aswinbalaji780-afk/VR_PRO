cmake_minimum_required(VERSION 3.16)
project(MinimalDX11Hook VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch MinHook from GitHub
include(FetchContent)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

# Create Shared Library (DLL)
add_library(MinimalDX11Hook SHARED main.cpp)

# Link MinHook, DirectX 11, DXGI, and D3DCompiler
target_link_libraries(MinimalDX11Hook PRIVATE minhook d3d11 dxgi d3dcompiler)

# Optimize for size and speed (Release builds)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(MinimalDX11Hook PRIVATE /O2 /Ob2)
endif()
