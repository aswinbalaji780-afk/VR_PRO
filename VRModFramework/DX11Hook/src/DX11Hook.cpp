#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include "DX11Hook.hpp"
#include "../../OpenXRLayer/include/OpenXRLayer.hpp"
#include "../../InputTranslator/include/InputTranslator.hpp"
#include <MinHook.h>
#include <iostream>
#include <fstream>
#include <string>

// --- FILE LOGGER ---
static std::ofstream g_logFile;
void Log(const std::string& msg) {
    if (!g_logFile.is_open()) {
        g_logFile.open("vr_log.txt", std::ios::out | std::ios::app);
    }
    if (g_logFile.is_open()) {
        g_logFile << msg << std::endl;
        g_logFile.flush();
    }
}

typedef HRESULT(WINAPI* PFN_Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(WINAPI* PFN_Present1)(IDXGISwapChain1* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters);
typedef void(WINAPI* PFN_DrawIndexed)(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

PFN_Present original_Present = nullptr;
PFN_Present1 original_Present1 = nullptr;
PFN_DrawIndexed original_DrawIndexed = nullptr;

namespace VRMod {


    static bool g_forceSplitScreen = false;

    // Globals for D3D11 Hooking
    static bool g_rendererInitialized = false;
    static ID3D11Device* g_device = nullptr;
    static ID3D11DeviceContext* g_context = nullptr;
    static ID3D11Texture2D* g_stereoTexture = nullptr; 

    bool InitializeRenderer(IDXGISwapChain* pSwapChain) {
        Log("InitializeRenderer triggered.");
        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device))) { Log("Failed GetDevice"); return false; }
        g_device->GetImmediateContext(&g_context);

        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        
        D3D11_TEXTURE2D_DESC desc; pBackBuffer->GetDesc(&desc);
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        g_device->CreateTexture2D(&desc, NULL, &g_stereoTexture);
        pBackBuffer->Release();

        VRMod::OpenXRLayer::Initialize();
        g_rendererInitialized = true;
        Log("InitializeRenderer SUCCESS.");
        return true;
    }

    void WINAPI hooked_DrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
        if (!VRMod::OpenXRLayer::IsInitialized() && !g_forceSplitScreen) {
            original_DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
            return;
        }

        UINT numViewports = 1;
        D3D11_VIEWPORT vp;
        pContext->RSGetViewports(&numViewports, &vp);

        // Only split main render passes (ignore small shadow maps / UI passes)
        if (numViewports > 0 && vp.Width > 1000.0f) {
            D3D11_VIEWPORT leftVp = vp;
            leftVp.Width /= 2.0f;
            
            D3D11_VIEWPORT rightVp = leftVp;
            rightVp.TopLeftX += leftVp.Width;

            // Draw Left Eye
            pContext->RSSetViewports(1, &leftVp);
            original_DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);

            // Draw Right Eye
            pContext->RSSetViewports(1, &rightVp);
            original_DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
            
            // Restore Original
            pContext->RSSetViewports(1, &vp);
        } else {
            original_DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
        }
    }

    void PerformVRRender(IDXGISwapChain* pSwapChain) {
        if (!g_rendererInitialized) if (!InitializeRenderer(pSwapChain)) return;
        if (!g_context) return;

        if (!VRMod::OpenXRLayer::IsInitialized() && !g_forceSplitScreen) return;

        ID3D11Texture2D* pBackBuffer = nullptr;
        if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) return;
            
        // The backbuffer is ALREADY split natively thanks to hooked_DrawIndexed.
        // We just need to copy it to g_stereoTexture for OpenXR to use!
        g_context->CopyResource(g_stereoTexture, pBackBuffer);
        
        pBackBuffer->Release();

        VRMod::OpenXRLayer::RenderFrame();
    }

    HRESULT WINAPI hooked_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        PerformVRRender(pSwapChain);
        return original_Present(pSwapChain, SyncInterval, Flags);
    }

    HRESULT WINAPI hooked_Present1(IDXGISwapChain1* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
        PerformVRRender(pSwapChain);
        return original_Present1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
    }

    DWORD WINAPI InitializeHookThread(LPVOID lpParam) {
        Log("InitializeHookThread spawned.");
        WNDCLASSEX windowClass = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, DefWindowProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DummyWindowDX11", NULL };
        RegisterClassEx(&windowClass);
        HWND dummyWindow = CreateWindow("DummyWindowDX11", "Dummy", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

        HMODULE libD3D11 = LoadLibraryA("d3d11.dll");
        if (!libD3D11) { Log("Failed to load d3d11.dll"); return 0; }
        
        typedef HRESULT(WINAPI* PFN_D3D11CreateDeviceAndSwapChain)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
        auto D3D11CreateDeviceAndSwapChain = (PFN_D3D11CreateDeviceAndSwapChain)GetProcAddress(libD3D11, "D3D11CreateDeviceAndSwapChain");

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 1; swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; swapChainDesc.OutputWindow = dummyWindow;
        swapChainDesc.SampleDesc.Count = 1; swapChainDesc.Windowed = TRUE;

        ID3D11Device* dummyDevice = nullptr; IDXGISwapChain* dummySwapChain = nullptr; ID3D11DeviceContext* dummyContext = nullptr; D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &swapChainDesc, &dummySwapChain, &dummyDevice, &featureLevel, &dummyContext);
        
        if (FAILED(hr)) { Log("D3D11CreateDeviceAndSwapChain failed."); return 0; }

        void** swapChainVTable = *reinterpret_cast<void***>(dummySwapChain);
        void* presentAddress = swapChainVTable[8];
        Log("Found Present address.");
        
        IDXGISwapChain1* dummySwapChain1 = nullptr;
        void* present1Address = nullptr;
        if (SUCCEEDED(dummySwapChain->QueryInterface(__uuidof(IDXGISwapChain1), (void**)&dummySwapChain1))) {
            void** swapChain1VTable = *reinterpret_cast<void***>(dummySwapChain1);
            present1Address = swapChain1VTable[22];
            dummySwapChain1->Release();
            Log("Found Present1 address.");
        }

        void** contextVTable = *reinterpret_cast<void***>(dummyContext);
        void* drawIndexedAddress = contextVTable[12];
        Log("Found DrawIndexed address.");

        dummyDevice->Release(); dummySwapChain->Release(); dummyContext->Release(); DestroyWindow(dummyWindow);

        MH_Initialize();
        MH_CreateHook(presentAddress, (LPVOID)&hooked_Present, reinterpret_cast<LPVOID*>(&original_Present));
        MH_EnableHook(presentAddress);
        Log("Hooked Present");
        
        if (present1Address) {
            MH_CreateHook(present1Address, (LPVOID)&hooked_Present1, reinterpret_cast<LPVOID*>(&original_Present1));
            MH_EnableHook(present1Address);
            Log("Hooked Present1");
        }

        MH_CreateHook(drawIndexedAddress, (LPVOID)&hooked_DrawIndexed, reinterpret_cast<LPVOID*>(&original_DrawIndexed));
        MH_EnableHook(drawIndexedAddress);
        Log("Hooked DrawIndexed");
        
        return 0;
    }

    // Globals to hold the DLL's own module handle so we can resolve vr_config.ini's path
    static HMODULE g_hModule = NULL;

    std::string DX11HookImpl::GetConfigPath() {
        char path[MAX_PATH];
        GetModuleFileNameA(g_hModule, path, MAX_PATH);
        std::string fullPath(path);
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            std::string dir = fullPath.substr(0, lastSlash);
            std::string sameDir = dir + "\\vr_config.ini";
            if (GetFileAttributesA(sameDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
                return sameDir; // Found in same folder (Distribution Mod)
            }
            return dir + "\\..\\..\\vr_config.ini"; // Development fallback
        }
        return ".\\vr_config.ini";
    }

    bool DX11HookImpl::Initialize(HMODULE hModule) {
        g_hModule = hModule;
        Log("DLL Attached. Starting Hook Thread.");

        char forceBuf[16];
        std::string configPath = GetConfigPath();
        if (GetPrivateProfileStringA("Debug", "ForceSplitScreen", "false", forceBuf, 16, configPath.c_str())) {
            if (_stricmp(forceBuf, "true") == 0) {
                g_forceSplitScreen = true;
                Log("DEBUG: ForceSplitScreen is enabled.");
            }
        }

        CreateThread(nullptr, 0, InitializeHookThread, nullptr, 0, nullptr);
        return true;
    }
    void DX11HookImpl::Shutdown() { MH_DisableHook(MH_ALL_HOOKS); MH_Uninitialize(); VRMod::OpenXRLayer::Shutdown(); }
    void* DX11HookImpl::GetStereoTexture() {
        return g_stereoTexture;
    }

    void* DX11HookImpl::GetDeviceContext() {
        return g_device; // For OpenXR session creation
    }

    void DX11HookImpl::CopyToOpenXRSwapchain(void* destTexture) {
        if (g_context && g_stereoTexture && destTexture) {
            g_context->CopyResource((ID3D11Resource*)destTexture, g_stereoTexture);
        }
    }


} // namespace VRMod

#include "../../MemoryManager/include/MemoryManager.hpp"
#include "../../InputTranslator/include/InputTranslator.hpp"

#include "../../GraphicsManager/include/GraphicsManager.hpp"

// Windows DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        VRMod::MemoryManager::Initialize(hModule);
        VRMod::InputTranslator::Initialize();
        VRMod::GraphicsManager::Get().Initialize(hModule);
        break;
    case DLL_PROCESS_DETACH:
        VRMod::GraphicsManager::Get().Shutdown();
        VRMod::InputTranslator::Shutdown();
        break;
    }
    return TRUE;
}

