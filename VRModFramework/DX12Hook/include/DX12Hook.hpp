#ifndef VRMOD_DX12HOOK_HPP
#define VRMOD_DX12HOOK_HPP

#ifdef _WIN32
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#endif
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
        void CopyToOpenXRSwapchain(void* destTexture) override;

    private:
        bool InitializeRendererDX12(ID3D12Device* device);
        void RenderSplitScreen(ID3D12Resource* pBackBuffer, ID3D12Resource* pDestTexture);
    };

} // namespace VRMod

#endif // VRMOD_DX12HOOK_HPP
