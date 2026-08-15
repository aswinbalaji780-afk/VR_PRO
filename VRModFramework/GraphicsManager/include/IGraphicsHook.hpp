#ifndef VRMOD_IGRAPHICSHOOK_HPP
#define VRMOD_IGRAPHICSHOOK_HPP

#ifdef _WIN32
#include <windows.h>
#else
// Define stub types for Apple/Linux to keep the interface generic
typedef void* HMODULE;
#endif

#include <string>

namespace VRMod {

    // Identifies the active graphics API
    enum class GraphicsApi {
        Unknown,
        D3D11,
        D3D12,
        Vulkan,
        OpenGL,
        Metal
    };

    // Generic interface for all graphics hooks
    class IGraphicsHook {
    public:
        virtual ~IGraphicsHook() = default;

        // Returns the API type this hook manages
        virtual GraphicsApi GetApiType() const = 0;

        // Initializes the MinHook interception
        virtual bool Initialize(HMODULE hModule) = 0;

        // Shuts down the hook and cleans up resources
        virtual void Shutdown() = 0;

        // Retrieves the split-screen stereoscopic texture handle.
        // For D3D11: returns ID3D11Texture2D*
        // For D3D12: returns ID3D12Resource*
        // For Vulkan: returns VkImage
        // For OpenGL: returns GLuint
        virtual void* GetStereoTexture() = 0;

        // Retrieves the active device handle.
        // For D3D11: returns ID3D11Device*
        // For D3D12: returns ID3D12CommandQueue* (Needed for OpenXR binding)
        // For Vulkan: returns VkDevice (or a custom struct holding Instance, PhysDev, Device, QueueIndex)
        // For OpenGL: returns HDC (or a custom struct holding HDC and HGLRC)
        virtual void* GetDeviceContext() = 0;
    };

} // namespace VRMod

#endif // VRMOD_IGRAPHICSHOOK_HPP
