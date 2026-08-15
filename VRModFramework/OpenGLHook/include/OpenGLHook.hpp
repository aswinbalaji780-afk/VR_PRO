#ifndef VRMOD_OPENGLHOOK_HPP
#define VRMOD_OPENGLHOOK_HPP

#include "../../GraphicsManager/include/IGraphicsHook.hpp"

namespace VRMod {

    class OpenGLHookImpl : public IGraphicsHook {
    public:
        OpenGLHookImpl() = default;
        ~OpenGLHookImpl() override { Shutdown(); }

        GraphicsApi GetApiType() const override { return GraphicsApi::OpenGL; }

        bool Initialize(HMODULE hModule) override;
        void Shutdown() override;

        void* GetStereoTexture() override;
        void* GetDeviceContext() override;
    };

} // namespace VRMod

#endif // VRMOD_OPENGLHOOK_HPP
