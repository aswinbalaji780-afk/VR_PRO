#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "VulkanHook.hpp"
#include "../../DX11Hook/include/MinHook.h"
#include <iostream>

// Minimal Vulkan typedefs so we don't need the full Vulkan SDK right now
typedef void* VkInstance;
typedef void* VkPhysicalDevice;
typedef void* VkDevice;
typedef void* VkQueue;
typedef void* VkSwapchainKHR;
typedef void (*PFN_vkVoidFunction)(void);

namespace VRMod {

    static VkInstance g_vkInstance = nullptr;
    static VkPhysicalDevice g_vkPhysicalDevice = nullptr;
    static VkDevice g_vkDevice = nullptr;
    static VkQueue g_vkQueue = nullptr;

    typedef PFN_vkVoidFunction(WINAPI* PFN_vkGetInstanceProcAddr)(VkInstance instance, const char* pName);
    typedef PFN_vkVoidFunction(WINAPI* PFN_vkGetDeviceProcAddr)(VkDevice device, const char* pName);

    static PFN_vkGetInstanceProcAddr original_vkGetInstanceProcAddr = nullptr;
    static PFN_vkGetDeviceProcAddr original_vkGetDeviceProcAddr = nullptr;

    // Real Vulkan structs for OpenXR
    struct VulkanDeviceContext {
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue queue;
    };
    static VulkanDeviceContext g_vkContext = {};

    // -------------------------------------------------------------
    // Trampoline functions
    // -------------------------------------------------------------

    // Dummy signature since we don't have vulkan.h included fully
    typedef int(WINAPI* PFN_vkQueuePresentKHR)(VkQueue queue, const void* pPresentInfo);
    static PFN_vkQueuePresentKHR original_vkQueuePresentKHR = nullptr;

    int WINAPI hooked_vkQueuePresentKHR(VkQueue queue, const void* pPresentInfo) {
        if (!g_vkQueue) {
            g_vkQueue = queue;
            g_vkContext.queue = queue;
            std::cout << "[VulkanHook] Captured VkQueue for presentation!\n";
        }
        
        // TODO: Extract swapchain images and submit to OpenXR

        return original_vkQueuePresentKHR(queue, pPresentInfo);
    }

    PFN_vkVoidFunction WINAPI hooked_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
        if (strcmp(pName, "vkQueuePresentKHR") == 0) {
            original_vkQueuePresentKHR = (PFN_vkQueuePresentKHR)original_vkGetDeviceProcAddr(device, pName);
            return (PFN_vkVoidFunction)hooked_vkQueuePresentKHR;
        }
        return original_vkGetDeviceProcAddr(device, pName);
    }

    PFN_vkVoidFunction WINAPI hooked_vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
        if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
            original_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)original_vkGetInstanceProcAddr(instance, pName);
            return (PFN_vkVoidFunction)hooked_vkGetDeviceProcAddr;
        }
        return original_vkGetInstanceProcAddr(instance, pName);
    }

    bool VulkanHookImpl::Initialize(HMODULE hModule) {
        HMODULE vulkanLib = GetModuleHandleA("vulkan-1.dll");
        if (!vulkanLib) return false;

        void* procAddr = GetProcAddress(vulkanLib, "vkGetInstanceProcAddr");
        if (!procAddr) return false;

        MH_Initialize();
        MH_CreateHook(procAddr, (LPVOID)&hooked_vkGetInstanceProcAddr, reinterpret_cast<LPVOID*>(&original_vkGetInstanceProcAddr));
        MH_EnableHook(MH_ALL_HOOKS);

        std::cout << "[VulkanHook] Hooked vkGetInstanceProcAddr successfully!\n";
        return true;
    }

    void VulkanHookImpl::Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
    }

    void* VulkanHookImpl::GetStereoTexture() {
        return nullptr;
    }

    void* VulkanHookImpl::GetDeviceContext() {
        return &g_vkContext;
    }

} // namespace VRMod
