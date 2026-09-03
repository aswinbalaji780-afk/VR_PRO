#include "GraphicsManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include "../../DX11Hook/include/DX11Hook.hpp"
#include "../../DX12Hook/include/DX12Hook.hpp"
#include "../../VulkanHook/include/VulkanHook.hpp"
#include "../../OpenGLHook/include/OpenGLHook.hpp"
#elif defined(__APPLE__)
#include "../../MetalHook/include/MetalHook.hpp"
#endif

namespace VRMod {

    GraphicsManager& GraphicsManager::Get() {
        static GraphicsManager instance;
        return instance;
    }

    static std::string GetConfigPath(HMODULE hModule) {
        char path[MAX_PATH];
        GetModuleFileNameA(hModule, path, MAX_PATH);
        std::string fullPath(path);
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            std::string sameDir = fullPath.substr(0, lastSlash) + "\\vr_config.ini";
            if (GetFileAttributesA(sameDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
                return sameDir;
            }
        }
        return ".\\vr_config.ini";
    }

    void WatchdogThread(GraphicsManager* manager) {
#ifdef _WIN32
        // 1. Check if user or profile explicitly forced an API in vr_config.ini
        char apiOverride[32] = { 0 };
        std::string cfgPath = GetConfigPath(manager->m_hModule);
        GetPrivateProfileStringA("Graphics", "API", "Auto", apiOverride, sizeof(apiOverride), cfgPath.c_str());

        if (_stricmp(apiOverride, "D3D12") == 0) {
            manager->m_activeApi = GraphicsApi::D3D12;
            manager->m_activeHook = std::make_unique<DX12HookImpl>();
        } else if (_stricmp(apiOverride, "D3D11") == 0) {
            manager->m_activeApi = GraphicsApi::D3D11;
            manager->m_activeHook = std::make_unique<DX11HookImpl>();
        } else if (_stricmp(apiOverride, "Vulkan") == 0) {
            manager->m_activeApi = GraphicsApi::Vulkan;
            manager->m_activeHook = std::make_unique<VulkanHookImpl>();
        } else if (_stricmp(apiOverride, "OpenGL") == 0) {
            manager->m_activeApi = GraphicsApi::OpenGL;
            manager->m_activeHook = std::make_unique<OpenGLHookImpl>();
        }

        if (manager->m_activeHook) {
            manager->m_activeHook->Initialize(manager->m_hModule);
            return;
        }
#endif

        // 2. Dynamic Auto-Detection:
        // - Vulkan games load vulkan-1.dll natively
        // - DX12 games load d3d12.dll (and may load d3d11.dll for overlays)
        // - DX11 games only load d3d11.dll
        // - OpenGL games load opengl32.dll
        while (true) {
#ifdef _WIN32
            if (GetModuleHandleA("vulkan-1.dll")) {
                manager->m_activeApi = GraphicsApi::Vulkan;
                manager->m_activeHook = std::make_unique<VulkanHookImpl>();
                break;
            }
            if (GetModuleHandleA("d3d12.dll")) {
                manager->m_activeApi = GraphicsApi::D3D12;
                manager->m_activeHook = std::make_unique<DX12HookImpl>();
                break;
            }
            if (GetModuleHandleA("d3d11.dll")) {
                manager->m_activeApi = GraphicsApi::D3D11;
                manager->m_activeHook = std::make_unique<DX11HookImpl>();
                break;
            }
            if (GetModuleHandleA("opengl32.dll")) {
                manager->m_activeApi = GraphicsApi::OpenGL;
                manager->m_activeHook = std::make_unique<OpenGLHookImpl>();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
#elif defined(__APPLE__)
            manager->m_activeApi = GraphicsApi::Metal;
            manager->m_activeHook = std::make_unique<MetalHookImpl>();
            break;
#endif
        }

        if (manager->m_activeHook) {
            manager->m_activeHook->Initialize(manager->m_hModule);
        }
    }

    bool GraphicsManager::Initialize(HMODULE hModule) {
        m_hModule = hModule;
        // Spawn a watchdog thread because Proxy DLLs load very early
        std::thread t(WatchdogThread, this);
        t.detach();
        return true;
    }

    void GraphicsManager::Shutdown() {
        if (m_activeHook) {
            m_activeHook->Shutdown();
            m_activeHook.reset();
        }
        m_activeApi = GraphicsApi::Unknown;
    }

    IGraphicsHook* GraphicsManager::GetActiveHook() const {
        return m_activeHook.get();
    }

    GraphicsApi GraphicsManager::GetActiveApi() const {
        return m_activeApi;
    }

} // namespace VRMod
