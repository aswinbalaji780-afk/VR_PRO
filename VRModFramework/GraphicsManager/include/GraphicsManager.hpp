#ifndef VRMOD_GRAPHICSMANAGER_HPP
#define VRMOD_GRAPHICSMANAGER_HPP

#include "IGraphicsHook.hpp"
#include <memory>

namespace VRMod {

    class GraphicsManager {
    public:
        // Returns the singleton instance
        static GraphicsManager& Get();

        // Detects the active API and initializes the appropriate hook
        bool Initialize(HMODULE hModule);

        // Shuts down the active hook
        void Shutdown();

        // Retrieves the active hook interface
        IGraphicsHook* GetActiveHook() const;

        // Retrieves the detected API type
        GraphicsApi GetActiveApi() const;

    private:
        GraphicsManager() = default;
        ~GraphicsManager() = default;

        // Prevent copying
        GraphicsManager(const GraphicsManager&) = delete;
        GraphicsManager& operator=(const GraphicsManager&) = delete;

        std::unique_ptr<IGraphicsHook> m_activeHook;
        GraphicsApi m_activeApi = GraphicsApi::Unknown;
        HMODULE m_hModule = nullptr;

        friend DWORD WINAPI WatchdogThread(LPVOID lpParam);
    };

} // namespace VRMod

#endif // VRMOD_GRAPHICSMANAGER_HPP
