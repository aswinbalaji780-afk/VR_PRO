#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include "OpenGLHook.hpp"
#include "../../DX11Hook/include/MinHook.h"
#include <iostream>

namespace VRMod {

    struct OpenGLDeviceContext {
        HDC hdc;
        HGLRC hglrc;
    };
    static OpenGLDeviceContext g_glContext = {};
    static GLuint g_stereoTexture = 0;

    typedef BOOL(WINAPI* PFN_wglSwapBuffers)(HDC);
    static PFN_wglSwapBuffers original_wglSwapBuffers = nullptr;

    BOOL WINAPI hooked_wglSwapBuffers(HDC hdc) {
        if (!g_glContext.hdc) {
            g_glContext.hdc = hdc;
            g_glContext.hglrc = wglGetCurrentContext();
            std::cout << "[OpenGLHook] Captured HDC and HGLRC!\n";
        }

        // TODO: glReadPixels or FBO capture for OpenXR stereo texture

        return original_wglSwapBuffers(hdc);
    }

    bool OpenGLHookImpl::Initialize(HMODULE hModule) {
        HMODULE glLib = GetModuleHandleA("opengl32.dll");
        if (!glLib) return false;

        void* procAddr = GetProcAddress(glLib, "wglSwapBuffers");
        if (!procAddr) return false;

        MH_Initialize();
        MH_CreateHook(procAddr, (LPVOID)&hooked_wglSwapBuffers, reinterpret_cast<LPVOID*>(&original_wglSwapBuffers));
        MH_EnableHook(MH_ALL_HOOKS);

        std::cout << "[OpenGLHook] Hooked wglSwapBuffers successfully!\n";
        return true;
    }

    void OpenGLHookImpl::Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
    }

    void* OpenGLHookImpl::GetStereoTexture() {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(g_stereoTexture));
    }

    void* OpenGLHookImpl::GetDeviceContext() {
        return &g_glContext;
    }

} // namespace VRMod
