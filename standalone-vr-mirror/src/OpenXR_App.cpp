#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <d3d11.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <iostream>
#include <vector>

// Note: This is a structural blueprint for a native OpenXR + DX11 application.
// A full, compilable production application requires extensive error checking (XR_SUCCEEDED macros),
// swapchain image enumeration, and view configuration queries.

class OpenXRApp {
private:
    XrInstance m_instance{XR_NULL_HANDLE};
    XrSystemId m_systemId{XR_NULL_SYSTEM_ID};
    XrSession m_session{XR_NULL_HANDLE};
    XrSpace m_appSpace{XR_NULL_HANDLE};
    
    // DirectX 11 state
    ID3D11Device* m_d3dDevice = nullptr;
    ID3D11DeviceContext* m_d3dContext = nullptr;

    // OpenXR Swapchain for submitting textures
    XrSwapchain m_swapchain{XR_NULL_HANDLE};
    std::vector<XrSwapchainImageD3D11KHR> m_swapchainImages;

public:
    bool Initialize(ID3D11Device* existingDevice) {
        m_d3dDevice = existingDevice;

        // 1. Create XrInstance
        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        strcpy_s(createInfo.applicationInfo.applicationName, "StandaloneVRMirror");
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        
        // We must enable the D3D11 extension to share textures with DirectX
        const char* extensions[] = { XR_KHR_D3D11_ENABLE_EXTENSION_NAME };
        createInfo.enabledExtensionCount = 1;
        createInfo.enabledExtensionNames = extensions;

        if (xrCreateInstance(&createInfo, &m_instance) != XR_SUCCESS) {
            std::cerr << "Failed to create OpenXR instance.\n";
            return false;
        }

        // 2. Get the VR Headset System ID
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        xrGetSystem(m_instance, &systemInfo, &m_systemId);

        // 3. Bind OpenXR to our DirectX 11 Device
        XrGraphicsBindingD3D11KHR d3d11Binding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
        d3d11Binding.device = m_d3dDevice;

        // 4. Create XrSession
        XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionCreateInfo.next = &d3d11Binding;
        sessionCreateInfo.systemId = m_systemId;

        if (xrCreateSession(m_instance, &sessionCreateInfo, &m_session) != XR_SUCCESS) {
            std::cerr << "Failed to create OpenXR session.\n";
            return false;
        }

        // 5. Create a Reference Space (e.g., LOCAL space for seated VR)
        XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
        xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_appSpace);

        // (Initialization of XrSwapchain and allocating m_swapchainImages would happen here)
        
        std::cout << "OpenXR Initialized and bound to DX11!\n";
        return true;
    }

    void RunFrameLoop(ID3D11Texture2D* shaderOutputTexture) {
        // --- The Core OpenXR Frame Loop ---
        
        // 1. Wait for the headset to tell us it's ready for a new frame
        // This physically blocks the thread to synchronize with the headset's refresh rate (e.g., 90Hz)
        XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState frameState{XR_TYPE_FRAME_STATE};
        xrWaitFrame(m_session, &waitInfo, &frameState);

        // 2. Begin the frame
        XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        xrBeginFrame(m_session, &beginInfo);

        // 3. Render only if the headset tells us to (it might be asleep)
        std::vector<XrCompositionLayerBaseHeader*> layers;
        XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        
        if (frameState.shouldRender) {
            // A. Acquire the next available swapchain image from OpenXR
            uint32_t imageIndex;
            XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            xrAcquireSwapchainImage(m_swapchain, &acquireInfo, &imageIndex);

            XrSwapchainImageWaitInfo waitImageInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            waitImageInfo.timeout = XR_INFINITE_DURATION;
            xrWaitSwapchainImage(m_swapchain, &waitImageInfo);

            // B. --- GRAPHICS WORK ---
            // Here you would use DX11 to copy your 'shaderOutputTexture' (the side-by-side frame) 
            // into the OpenXR swapchain image: m_swapchainImages[imageIndex].texture
            // Example: m_d3dContext->CopyResource(m_swapchainImages[imageIndex].texture, shaderOutputTexture);

            // C. Release the swapchain image back to OpenXR
            XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(m_swapchain, &releaseInfo);

            // D. Populate the Composition Layer with the rendered views
            // (You would calculate the FOV and poses for the Left and Right eyes here)
            projectionLayer.space = m_appSpace;
            // projectionLayer.views = ... (Array of Left and Right XrCompositionLayerProjectionView)
            
            layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&projectionLayer));
        }

        // 4. End the frame and submit the layers to the VR headset display!
        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = static_cast<uint32_t>(layers.size());
        endInfo.layers = layers.data();
        
        xrEndFrame(m_session, &endInfo);
    }

    void Shutdown() {
        if (m_appSpace != XR_NULL_HANDLE) xrDestroySpace(m_appSpace);
        if (m_session != XR_NULL_HANDLE) xrDestroySession(m_session);
        if (m_instance != XR_NULL_HANDLE) xrDestroyInstance(m_instance);
    }
};
