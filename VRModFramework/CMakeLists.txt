cmake_minimum_required(VERSION 3.24)
project(VRModFramework VERSION 1.0.0 LANGUAGES C CXX)

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
    InputTranslator/src/InputTranslator.cpp
    MemoryManager/src/MemoryManager.cpp
    OpenXRLayer/src/OpenXRLayer.cpp
)

if(WIN32)
    list(APPEND SOURCES 
        DX11Hook/src/DX11Hook.cpp
        DX12Hook/src/DX12Hook.cpp
        VulkanHook/src/VulkanHook.cpp
        OpenGLHook/src/OpenGLHook.cpp
        Proxy/DXGIWrapper.cpp
    )
endif()

if(APPLE)
    list(APPEND SOURCES MetalHook/src/MetalHook.mm)
endif()

# Fetch MinHook from GitHub
include(FetchContent)
FetchContent_Declare(
    minhook
    URL https://github.com/TsudaKageyu/minhook/archive/refs/tags/v1.3.3.zip
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(minhook)

# Compile MinHook statically from source because the official repo lacks CMakeLists.txt
if(NOT TARGET minhook)
    add_library(minhook STATIC
        ${minhook_SOURCE_DIR}/src/buffer.c
        ${minhook_SOURCE_DIR}/src/hook.c
        ${minhook_SOURCE_DIR}/src/trampoline.c
        ${minhook_SOURCE_DIR}/src/hde/hde32.c
        ${minhook_SOURCE_DIR}/src/hde/hde64.c
    )
    target_include_directories(minhook PUBLIC ${minhook_SOURCE_DIR}/include)
endif()


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
target_include_directories(VRModFramework PUBLIC 
    ${INCLUDE_DIRS}
    ${minhook_SOURCE_DIR}/include
)

# Link dependencies (openxr_loader is built by the fetched OpenXR SDK)
if(WIN32)
    target_link_libraries(VRModFramework PRIVATE 
        minhook 
        d3d11 
        d3d12
        dxgi 
        d3dcompiler 
        opengl32
        openxr_loader
    )
    
    # Set output name to dxgi to act as a proxy DLL on Windows
    set_target_properties(VRModFramework PROPERTIES
        OUTPUT_NAME "dxgi"
        LINKER_LANGUAGE CXX
    )
elseif(APPLE)
    # Link Apple frameworks natively
    find_library(METAL_LIBRARY Metal)
    find_library(FOUNDATION_LIBRARY Foundation)
    
    target_link_libraries(VRModFramework PRIVATE 
        ${METAL_LIBRARY}
        ${FOUNDATION_LIBRARY}
        openxr_loader
    )
    
    set_target_properties(VRModFramework PROPERTIES
        OUTPUT_NAME "VRMod"
        LINKER_LANGUAGE CXX
    )
endif()

# Copy the openxr_loader.dll/dylib next to our plugin so electron-builder can find it
add_custom_command(TARGET VRModFramework POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:openxr_loader>
    $<TARGET_FILE_DIR:VRModFramework>
)
