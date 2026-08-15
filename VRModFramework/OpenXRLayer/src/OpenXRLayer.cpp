#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#define XR_USE_GRAPHICS_API_OPENGL_WIN32
// #define XR_USE_GRAPHICS_API_VULKAN // Uncomment when Vulkan SDK is fully integrated

#include "OpenXRLayer.hpp"
#include "../../GraphicsManager/include/GraphicsManager.hpp"
#include "../../InputTranslator/include/InputTranslator.hpp"
#include "../../MemoryManager/include/MemoryManager.hpp"
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <GL/gl.h>
// #include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <iostream>
#include <vector>

namespace VRMod {
namespace OpenXRLayer {

    static XrInstance g_instance = XR_NULL_HANDLE;
    static XrSystemId g_systemId = XR_NULL_SYSTEM_ID;
    static XrSession g_session = XR_NULL_HANDLE;
    static XrSpace g_appSpace = XR_NULL_HANDLE;
    static XrSwapchain g_swapchain = XR_NULL_HANDLE;
    static std::vector<XrSwapchainImageD3D11KHR> g_swapchainImages;
    static ID3D11DeviceContext* g_d3dContext = nullptr;

    // Input Actions
    static XrActionSet g_actionSet = XR_NULL_HANDLE;
    static XrAction g_actionAttack = XR_NULL_HANDLE;
    static XrAction g_actionDeflect = XR_NULL_HANDLE;
    static XrAction g_actionRightPose = XR_NULL_HANDLE;
    static XrAction g_actionLeftPose = XR_NULL_HANDLE;
    
    // Hand Spaces
    static XrSpace g_rightHandSpace = XR_NULL_HANDLE;
    static XrSpace g_leftHandSpace = XR_NULL_HANDLE;

    bool InitializeInput() {
        // 1. Create Action Set
        XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
        strcpy_s(actionSetInfo.actionSetName, "universal_vr_actions");
        strcpy_s(actionSetInfo.localizedActionSetName, "Universal VR Controls");
        if (xrCreateActionSet(g_instance, &actionSetInfo, &g_actionSet) != XR_SUCCESS) return false;

        // 2. Create Actions (Attack and Deflect)
        XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
        actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
        
        strcpy_s(actionInfo.actionName, "attack");
        strcpy_s(actionInfo.localizedActionName, "Attack");
        xrCreateAction(g_actionSet, &actionInfo, &g_actionAttack);

        strcpy_s(actionInfo.actionName, "deflect");
        strcpy_s(actionInfo.localizedActionName, "Deflect");
        xrCreateAction(g_actionSet, &actionInfo, &g_actionDeflect);

        // Motion Controller Poses
        actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
        strcpy_s(actionInfo.actionName, "right_hand_pose");
        strcpy_s(actionInfo.localizedActionName, "Right Hand Pose");
        xrCreateAction(g_actionSet, &actionInfo, &g_actionRightPose);

        strcpy_s(actionInfo.actionName, "left_hand_pose");
        strcpy_s(actionInfo.localizedActionName, "Left Hand Pose");
        xrCreateAction(g_actionSet, &actionInfo, &g_actionLeftPose);

        // 3. Suggest Bindings for Oculus Touch (as an example)
        XrPath interactionProfilePath, attackPath, deflectPath, rightPosePath, leftPosePath;
        xrStringToPath(g_instance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePath);
        xrStringToPath(g_instance, "/user/hand/right/input/trigger/value", &attackPath);
        xrStringToPath(g_instance, "/user/hand/left/input/trigger/value", &deflectPath);
        xrStringToPath(g_instance, "/user/hand/right/input/grip/pose", &rightPosePath);
        xrStringToPath(g_instance, "/user/hand/left/input/grip/pose", &leftPosePath);

        std::vector<XrActionSuggestedBinding> bindings = {
            {g_actionAttack, attackPath},
            {g_actionDeflect, deflectPath},
            {g_actionRightPose, rightPosePath},
            {g_actionLeftPose, leftPosePath}
        };

        XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestedBindings.interactionProfile = interactionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        xrSuggestInteractionProfileBindings(g_instance, &suggestedBindings);

        // 4. Create Action Spaces for Hands
        XrActionSpaceCreateInfo spaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        spaceInfo.action = g_actionRightPose;
        spaceInfo.poseInActionSpace.orientation.w = 1.0f;
        xrCreateActionSpace(g_session, &spaceInfo, &g_rightHandSpace);

        spaceInfo.action = g_actionLeftPose;
        xrCreateActionSpace(g_session, &spaceInfo, &g_leftHandSpace);

        // Attach action set to session
        XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        attachInfo.countActionSets = 1;
        attachInfo.actionSets = &g_actionSet;
        xrAttachSessionActionSets(g_session, &attachInfo);

        VRMod::InputTranslator::Initialize();
        return true;
    }

    bool Initialize() {
        ID3D11Device* d3dDevice = VRMod::DX11Hook::GetDevice();
        if (!d3dDevice) return false;
        d3dDevice->GetImmediateContext(&g_d3dContext);

        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        strcpy_s(createInfo.applicationInfo.applicationName, "VRModFramework");
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        const char* extensions[] = { XR_KHR_D3D11_ENABLE_EXTENSION_NAME };
        createInfo.enabledExtensionCount = 1; createInfo.enabledExtensionNames = extensions;

        if (xrCreateInstance(&createInfo, &g_instance) != XR_SUCCESS) return false;

        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        if (xrGetSystem(g_instance, &systemInfo, &g_systemId) != XR_SUCCESS) return false;

        XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};

        XrGraphicsBindingD3D11KHR d3d11Binding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
        XrGraphicsBindingD3D12KHR d3d12Binding{XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};
        XrGraphicsBindingOpenGLWin32KHR glBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};

        auto api = GraphicsManager::Get().GetActiveApi();
        auto hook = GraphicsManager::Get().GetActiveHook();

        if (api == GraphicsApi::D3D11) {
            d3d11Binding.device = (ID3D11Device*)hook->GetDeviceContext();
            sessionCreateInfo.next = &d3d11Binding;
        } else if (api == GraphicsApi::D3D12) {
            d3d12Binding.device = (ID3D12Device*)hook->GetDeviceContext(); // OpenXR needs device, queue is needed for submit
            sessionCreateInfo.next = &d3d12Binding;
        } else if (api == GraphicsApi::OpenGL) {
            // OpenGL binding needs HDC and HGLRC
            // glBinding.hDC = ...
            // glBinding.hGLRC = ...
            sessionCreateInfo.next = &glBinding;
        } else {
            return false; // Unsupported API
        }

        sessionCreateInfo.systemId = g_systemId;
        if (xrCreateSession(g_instance, &sessionCreateInfo, &g_session) != XR_SUCCESS) return false;

        XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
        xrCreateReferenceSpace(g_session, &spaceCreateInfo, &g_appSpace);

        // Initialize VR Inputs!
        InitializeInput();

        std::cout << "[OpenXRLayer] Initialized and bound to Graphics Hook!\n";
        return true;
    }

    void PollInputs() {
        XrActiveActionSet activeActionSet{};
        activeActionSet.actionSet = g_actionSet;
        activeActionSet.subactionPath = XR_NULL_PATH;

        XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;
        xrSyncActions(g_session, &syncInfo);

        XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        XrActionStateBoolean attackState{XR_TYPE_ACTION_STATE_BOOLEAN};
        getInfo.action = g_actionAttack;
        xrGetActionStateBoolean(g_session, &getInfo, &attackState);
        VRMod::InputTranslator::OnActionAttack(attackState.currentState);

        XrActionStateBoolean deflectState{XR_TYPE_ACTION_STATE_BOOLEAN};
        getInfo.action = g_actionDeflect;
        xrGetActionStateBoolean(g_session, &getInfo, &deflectState);
        VRMod::InputTranslator::OnActionDeflect(deflectState.currentState);
    }

    void UpdateHeadTracking(XrTime displayTime) {
        XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
        if (xrLocateSpace(g_appSpace, g_appSpace, displayTime, &spaceLocation) == XR_SUCCESS) {
            if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                
                // Positional Tracking
                float x = spaceLocation.pose.position.x;
                float y = spaceLocation.pose.position.y;
                float z = spaceLocation.pose.position.z;
                
                // Rotational Tracking (Quaternion to Euler Angles conversion)
                float qx = spaceLocation.pose.orientation.x;
                float qy = spaceLocation.pose.orientation.y;
                float qz = spaceLocation.pose.orientation.z;
                float qw = spaceLocation.pose.orientation.w;

                // Standard math to convert Quaternion to Pitch, Yaw, Roll (in radians)
                float sinr_cosp = 2.0f * (qw * qx + qy * qz);
                float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
                float roll = atan2(sinr_cosp, cosr_cosp);

                float sinp = sqrt(1.0f + 2.0f * (qw * qy - qx * qz));
                float cosp = sqrt(1.0f - 2.0f * (qw * qy - qx * qz));
                float pitch = 2.0f * atan2(sinp, cosp) - 3.14159f / 2.0f;

                float siny_cosp = 2.0f * (qw * qz + qx * qy);
                float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
                float yaw = atan2(siny_cosp, cosy_cosp);

                static bool s_configLoaded = false;
                static bool s_isMouseMode = false;
                static float s_sensX = 1000.0f;
                static float s_sensY = 1000.0f;
                static bool s_invertY = false;

                if (!s_configLoaded) {
                    std::string configPath = VRMod::DX11Hook::GetConfigPath();
                    char modeBuf[32];
                    GetPrivateProfileStringA("HeadTracking", "Mode", "Memory", modeBuf, 32, configPath.c_str());
                    if (_stricmp(modeBuf, "Mouse") == 0) s_isMouseMode = true;

                    char sensXBuf[32], sensYBuf[32], invBuf[16];
                    GetPrivateProfileStringA("HeadTracking", "MouseSensitivityX", "1000.0", sensXBuf, 32, configPath.c_str());
                    GetPrivateProfileStringA("HeadTracking", "MouseSensitivityY", "1000.0", sensYBuf, 32, configPath.c_str());
                    GetPrivateProfileStringA("HeadTracking", "InvertY", "false", invBuf, 16, configPath.c_str());
                    
                    s_sensX = std::stof(sensXBuf);
                    s_sensY = std::stof(sensYBuf);
                    s_invertY = (_stricmp(invBuf, "true") == 0);
                    s_configLoaded = true;
                }

                if (s_isMouseMode) {
                    static float s_prevPitch = 0.0f;
                    static float s_prevYaw = 0.0f;
                    static bool s_firstFrame = true;

                    if (!s_firstFrame) {
                        float deltaPitch = pitch - s_prevPitch;
                        float deltaYaw = yaw - s_prevYaw;
                        
                        // Handle wraparound at -pi and pi
                        if (deltaYaw > 3.14159f) deltaYaw -= 2.0f * 3.14159f;
                        if (deltaYaw < -3.14159f) deltaYaw += 2.0f * 3.14159f;
                        
                        VRMod::InputTranslator::UpdateMouseLook(deltaPitch, deltaYaw, s_sensX, s_sensY, s_invertY);
                    }
                    s_prevPitch = pitch;
                    s_prevYaw = yaw;
                    s_firstFrame = false;
                } else {
                    // Memory Mode (6DOF)
                    VRMod::MemoryManager::UpdateCamera(x, y, z, pitch, yaw, roll);
                }
            }
        }
    }

    void UpdateHandTracking(XrTime displayTime) {
        if (g_rightHandSpace == XR_NULL_HANDLE || g_leftHandSpace == XR_NULL_HANDLE) return;

        XrSpaceLocation rightSpaceLocation{XR_TYPE_SPACE_LOCATION};
        XrSpaceLocation leftSpaceLocation{XR_TYPE_SPACE_LOCATION};
        
        bool rightValid = (xrLocateSpace(g_rightHandSpace, g_appSpace, displayTime, &rightSpaceLocation) == XR_SUCCESS) &&
                          ((rightSpaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0);
                          
        bool leftValid = (xrLocateSpace(g_leftHandSpace, g_appSpace, displayTime, &leftSpaceLocation) == XR_SUCCESS) &&
                         ((leftSpaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0);

        if (rightValid && leftValid) {
            float rx = rightSpaceLocation.pose.position.x;
            float ry = rightSpaceLocation.pose.position.y;
            float rz = rightSpaceLocation.pose.position.z;

            // Simplified Quat to Euler (omitted full logic for brevity, just passing zeros for rotation right now)
            float rp = 0.0f, ryaw = 0.0f, rroll = 0.0f;

            float lx = leftSpaceLocation.pose.position.x;
            float ly = leftSpaceLocation.pose.position.y;
            float lz = leftSpaceLocation.pose.position.z;
            float lp = 0.0f, lyaw = 0.0f, lroll = 0.0f;

            VRMod::MemoryManager::UpdateHands(rx, ry, rz, rp, ryaw, rroll, lx, ly, lz, lp, lyaw, lroll);
        }
    }

    bool IsInitialized() {
        return (g_session != XR_NULL_HANDLE && g_d3dContext != nullptr);
    }

    void RenderFrame() {
        if (!IsInitialized()) return;
        
        // Read the controllers every frame
        PollInputs();

        ID3D11Texture2D* shaderOutputTexture = VRMod::DX11Hook::GetStereoTexture();
        if (!shaderOutputTexture) return;

        XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState frameState{XR_TYPE_FRAME_STATE};
        if (xrWaitFrame(g_session, &waitInfo, &frameState) != XR_SUCCESS) return;

        // Update the game's camera and hands with our physical position before rendering!
        UpdateHeadTracking(frameState.predictedDisplayTime);
        UpdateHandTracking(frameState.predictedDisplayTime);

        XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        xrBeginFrame(g_session, &beginInfo);

        std::vector<XrCompositionLayerBaseHeader*> layers;
        XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};

        if (frameState.shouldRender && g_swapchain != XR_NULL_HANDLE) {
            uint32_t imageIndex;
            XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            xrAcquireSwapchainImage(g_swapchain, &acquireInfo, &imageIndex);

            XrSwapchainImageWaitInfo waitImageInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            waitImageInfo.timeout = XR_INFINITE_DURATION;
            xrWaitSwapchainImage(g_swapchain, &waitImageInfo);

            g_d3dContext->CopyResource(g_swapchainImages[imageIndex].texture, shaderOutputTexture);

            XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(g_swapchain, &releaseInfo);

            projectionLayer.space = g_appSpace;
            layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&projectionLayer));
        }

        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = static_cast<uint32_t>(layers.size());
        endInfo.layers = layers.data();
        xrEndFrame(g_session, &endInfo);
    }

    void Shutdown() {
        VRMod::InputTranslator::Shutdown();
        if (g_actionAttack != XR_NULL_HANDLE) xrDestroyAction(g_actionAttack);
        if (g_actionDeflect != XR_NULL_HANDLE) xrDestroyAction(g_actionDeflect);
        if (g_actionSet != XR_NULL_HANDLE) xrDestroyActionSet(g_actionSet);
        if (g_swapchain != XR_NULL_HANDLE) xrDestroySwapchain(g_swapchain);
        if (g_appSpace != XR_NULL_HANDLE) xrDestroySpace(g_appSpace);
        if (g_session != XR_NULL_HANDLE) xrDestroySession(g_session);
        if (g_instance != XR_NULL_HANDLE) xrDestroyInstance(g_instance);
    }

} // namespace OpenXRLayer
} // namespace VRMod
