#ifndef VRMOD_METALHOOK_HPP
#define VRMOD_METALHOOK_HPP

#include "../../GraphicsManager/include/IGraphicsHook.hpp"

#ifdef __APPLE__

namespace VRMod {

    class MetalHookImpl : public IGraphicsHook {
    public:
        MetalHookImpl() = default;
        ~MetalHookImpl() override { Shutdown(); }

        GraphicsApi GetApiType() const override { return GraphicsApi::Metal; }

        bool Initialize(HMODULE hModule) override;
        void Shutdown() override;

        void* GetStereoTexture() override;
        void* GetDeviceContext() override;
        void CopyToOpenXRSwapchain(void* destTexture) override;
    };

} // namespace VRMod

#endif // __APPLE__

#endif // VRMOD_METALHOOK_HPP
