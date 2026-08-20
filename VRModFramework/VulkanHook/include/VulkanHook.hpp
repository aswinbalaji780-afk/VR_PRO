#ifndef VRMOD_VULKANHOOK_HPP
#define VRMOD_VULKANHOOK_HPP

#include "../../GraphicsManager/include/IGraphicsHook.hpp"

namespace VRMod {

    class VulkanHookImpl : public IGraphicsHook {
    public:
        VulkanHookImpl() = default;
        ~VulkanHookImpl() override { Shutdown(); }

        GraphicsApi GetApiType() const override { return GraphicsApi::Vulkan; }

        bool Initialize(HMODULE hModule) override;
        void Shutdown() override;

        void* GetStereoTexture() override;
        void* GetDeviceContext() override;
        void CopyToOpenXRSwapchain(void* destTexture) override;
    };

} // namespace VRMod

#endif // VRMOD_VULKANHOOK_HPP
