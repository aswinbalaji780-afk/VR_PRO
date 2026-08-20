#include "MetalHook.hpp"
#include <iostream>

#ifdef __APPLE__
#import <Metal/Metal.h>
#import <objc/runtime.h>

namespace VRMod {

    struct MetalDeviceContext {
        id<MTLDevice> device;
        id<MTLCommandQueue> queue;
    };
    static MetalDeviceContext g_metalContext = {};
    static id<MTLTexture> g_stereoTexture = nil;

    // Pointer to original method
    static id<MTLCommandBuffer> (*original_commandBuffer)(id, SEL) = nullptr;

    // Swizzled method
    id<MTLCommandBuffer> swizzled_commandBuffer(id self, SEL _cmd) {
        if (!g_metalContext.queue) {
            g_metalContext.queue = self;
            g_metalContext.device = [self device];
            std::cout << "[MetalHook] Intercepted MTLCommandQueue!\n";
        }

        // Call the original implementation
        id<MTLCommandBuffer> buffer = original_commandBuffer(self, _cmd);
        
        // TODO: In a full implementation, we'd add an MTLRenderCommandEncoder 
        // to this buffer to copy the game's final render pass to g_stereoTexture
        
        return buffer;
    }

    bool MetalHookImpl::Initialize(HMODULE hModule) {
        Class queueClass = NSClassFromString(@"_MTLCommandQueue"); // Metal internal command queue class
        if (!queueClass) return false;

        SEL selector = @selector(commandBuffer);
        Method originalMethod = class_getInstanceMethod(queueClass, selector);
        if (!originalMethod) return false;

        original_commandBuffer = (id<MTLCommandBuffer> (*)(id, SEL))method_getImplementation(originalMethod);
        method_setImplementation(originalMethod, (IMP)swizzled_commandBuffer);

        std::cout << "[MetalHook] Swizzled [MTLCommandQueue commandBuffer] successfully!\n";
        return true;
    }

    void MetalHookImpl::Shutdown() {
        if (original_commandBuffer) {
            Class queueClass = NSClassFromString(@"_MTLCommandQueue");
            SEL selector = @selector(commandBuffer);
            Method originalMethod = class_getInstanceMethod(queueClass, selector);
            method_setImplementation(originalMethod, (IMP)original_commandBuffer);
        }
    }

    void* MetalHookImpl::GetStereoTexture() {
        return (__bridge void*)g_stereoTexture;
    }

    void* MetalHookImpl::GetDeviceContext() {
        return &g_metalContext;
    }

    void MetalHookImpl::CopyToOpenXRSwapchain(void* destTexture) {
        // Implementation pending
    }

} // namespace VRMod
#endif // __APPLE__
