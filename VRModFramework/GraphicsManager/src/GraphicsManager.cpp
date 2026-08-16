#include "GraphicsManager.hpp"
#include "../../DX11Hook/include/DX11Hook.hpp"
// #include "../../DX12Hook/include/DX12Hook.hpp"
// #include "../../VulkanHook/include/VulkanHook.hpp"
// #include "../../OpenGLHook/include/OpenGLHook.hpp"
#include <iostream>

namespace VRMod {

    GraphicsManager& GraphicsManager::Get() {
        static GraphicsManager instance;
        return instance;
    }

    DWORD WINAPI WatchdogThread(LPVOID lpParam) {
        GraphicsManager* manager = (GraphicsManager*)lpParam;
        
        while (true) {
#ifdef _WIN32
            if (GetModuleHandleA("d3d11.dll")) {
                manager->m_activeApi = GraphicsApi::D3D11;
                manager->m_activeHook = std::make_unique<DX11HookImpl>();
                break;
            }
#elif defined(__APPLE__)
            manager->m_activeApi = GraphicsApi::Metal;
            // manager->m_activeHook = std::make_unique<MetalHookImpl>();
            break;
#endif
            Sleep(100);
        }

        if (manager->m_activeHook) {
            manager->m_activeHook->Initialize(manager->m_hModule);
        }
        return 0;
    }

    bool GraphicsManager::Initialize(HMODULE hModule) {
        m_hModule = hModule;
        // Spawn a watchdog thread because Proxy DLLs load very early,
        // often before the game loads d3d11.dll!
        CreateThread(nullptr, 0, WatchdogThread, this, 0, nullptr);
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
