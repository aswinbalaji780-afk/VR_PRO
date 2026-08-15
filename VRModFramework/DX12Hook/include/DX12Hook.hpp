#ifndef VRMOD_DX12HOOK_HPP
#define VRMOD_DX12HOOK_HPP

#include "../../GraphicsManager/include/IGraphicsHook.hpp"

namespace VRMod {

    class DX12HookImpl : public IGraphicsHook {
    public:
        DX12HookImpl() = default;
        ~DX12HookImpl() override { Shutdown(); }

        GraphicsApi GetApiType() const override { return GraphicsApi::D3D12; }

        bool Initialize(HMODULE hModule) override;
        void Shutdown() override;

        void* GetStereoTexture() override;
        void* GetDeviceContext() override;
    };

} // namespace VRMod

#endif // VRMOD_DX12HOOK_HPP
