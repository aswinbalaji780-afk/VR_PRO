cmake_minimum_required(VERSION 3.16)
project(VRModFramework VERSION 1.0.0 LANGUAGES CXX)

# Suppress annoying developer and deprecation warnings from third-party dependencies (MinHook/OpenXR)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define include directories
set(INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}/GraphicsManager/include
    ${CMAKE_CURRENT_SOURCE_DIR}/DX11Hook/include
    ${CMAKE_CURRENT_SOURCE_DIR}/DX12Hook/include
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanHook/include
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenGLHook/include
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenXRLayer/include
    ${CMAKE_CURRENT_SOURCE_DIR}/InputTranslator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/MemoryManager/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Proxy
)

if(APPLE)
    list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/MetalHook/include)
endif()

# Define source files
set(SOURCES
    GraphicsManager/src/GraphicsManager.cpp
    DX11Hook/src/DX11Hook.cpp
    DX12Hook/src/DX12Hook.cpp
    VulkanHook/src/VulkanHook.cpp
    OpenGLHook/src/OpenGLHook.cpp
    OpenXRLayer/src/OpenXRLayer.cpp
    InputTranslator/src/InputTranslator.cpp
    MemoryManager/src/MemoryManager.cpp
    Proxy/DXGIWrapper.cpp
)

if(APPLE)
    list(APPEND SOURCES MetalHook/src/MetalHook.mm)
endif()

# Fetch MinHook from GitHub
include(FetchContent)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

# Fetch OpenXR SDK from Khronos Group
# We inject a minimum policy version to fix the "CMake < 3.5" error from OpenXR's older CMakeLists
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
    openxr
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
    GIT_TAG        release-1.0.30
)
FetchContent_MakeAvailable(openxr)

# Create the shared library (DLL)
add_library(VRModFramework SHARED ${SOURCES})

# Include directories
target_include_directories(VRModFramework PUBLIC ${INCLUDE_DIRS})

# Link dependencies (openxr_loader is built by the fetched OpenXR SDK)
target_link_libraries(VRModFramework PRIVATE 
    minhook 
    d3d11 
    d3d12
    dxgi 
    d3dcompiler 
    opengl32
    openxr_loader
)

# Set output name to dxgi to act as a proxy DLL
set_target_properties(VRModFramework PROPERTIES
    OUTPUT_NAME "dxgi"
    LINKER_LANGUAGE CXX
)
