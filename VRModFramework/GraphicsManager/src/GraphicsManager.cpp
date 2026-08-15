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

    bool GraphicsManager::Initialize(HMODULE hModule) {
        // Auto-detect graphics API by checking loaded modules
        // Note: Some games load multiple APIs. The order here determines priority.
        
        // Let's default to DX11 for testing, but in a real implementation we would
        // check GetModuleHandleA("d3d11.dll") etc.
        
#ifdef _WIN32
        if (GetModuleHandleA("d3d11.dll")) {
            m_activeApi = GraphicsApi::D3D11;
            m_activeHook = std::make_unique<DX11HookImpl>();
        } else {
            return false;
        }
#elif defined(__APPLE__)
        // macOS always uses Metal
        m_activeApi = GraphicsApi::Metal;
        // m_activeHook = std::make_unique<MetalHookImpl>(); // Will be included in MetalHook.mm
#else
        return false;
#endif

        if (m_activeHook) {
            return m_activeHook->Initialize(hModule);
        }

        return false;
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
