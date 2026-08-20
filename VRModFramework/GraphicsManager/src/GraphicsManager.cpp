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

    void WatchdogThread(GraphicsManager* manager) {
        while (true) {
#ifdef _WIN32
            if (GetModuleHandleA("d3d11.dll")) {
                manager->m_activeApi = GraphicsApi::D3D11;
                manager->m_activeHook = std::make_unique<DX11HookImpl>();
                break;
            }
            if (GetModuleHandleA("d3d12.dll")) {
                manager->m_activeApi = GraphicsApi::D3D12;
                manager->m_activeHook = std::make_unique<DX12HookImpl>();
                break;
            }
            if (GetModuleHandleA("vulkan-1.dll")) {
                manager->m_activeApi = GraphicsApi::Vulkan;
                manager->m_activeHook = std::make_unique<VulkanHookImpl>();
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
