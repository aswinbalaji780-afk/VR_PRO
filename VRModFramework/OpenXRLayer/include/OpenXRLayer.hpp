#ifndef VRMOD_OPENXRLAYER_HPP
#define VRMOD_OPENXRLAYER_HPP

namespace VRMod {
namespace OpenXRLayer {

    // Initializes OpenXR session and connects to the runtime
    bool Initialize();

    // Returns whether the OpenXR session is initialized
    bool IsInitialized();

    // Renders the intercepted frame to the VR headset
    void RenderFrame();

    // Shuts down OpenXR session
    void Shutdown();

} // namespace OpenXRLayer
} // namespace VRMod

#endif // VRMOD_OPENXRLAYER_HPP
