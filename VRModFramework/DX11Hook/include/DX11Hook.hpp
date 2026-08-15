#ifndef VRMOD_DX11HOOK_HPP
#define VRMOD_DX11HOOK_HPP

#include <d3d11.h>
#include <string>

#include "../../GraphicsManager/include/IGraphicsHook.hpp"

namespace VRMod {

    class DX11HookImpl : public IGraphicsHook {
    public:
        DX11HookImpl() = default;
        ~DX11HookImpl() override { Shutdown(); }

        GraphicsApi GetApiType() const override { return GraphicsApi::D3D11; }
        
        bool Initialize(HMODULE hModule) override;
        void Shutdown() override;
        
        void* GetStereoTexture() override;
        void* GetDeviceContext() override;

        // Legacy accessors
        static std::string GetConfigPath();
    };

} // namespace VRMod

#endif // VRMOD_DX11HOOK_HPP
