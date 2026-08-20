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
    static ID3D11ShaderResourceView* g_copySRV = nullptr;
    static ID3D11VertexShader* g_vertexShader = nullptr;
    static ID3D11PixelShader* g_pixelShader = nullptr;
    static ID3D11SamplerState* g_samplerState = nullptr;
    
    // Solid Rendering States
    static ID3D11RasterizerState* g_rasterState = nullptr;
    static ID3D11DepthStencilState* g_depthState = nullptr;
    static ID3D11BlendState* g_blendState = nullptr;

    const char* g_vsCode = R"(
    struct VS_OUTPUT { float4 pos : SV_POSITION; float2 tex : TEXCOORD0; };
    VS_OUTPUT main(uint id : SV_VertexID) {
        VS_OUTPUT output;
        output.tex = float2((id << 1) & 2, id & 2);
        output.pos = float4(output.tex * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
        return output;
    })";

    const char* g_psCode = R"(
    Texture2D shaderTexture : register(t0);
    SamplerState SampleType : register(s0);
    struct PixelInputType { float4 position : SV_POSITION; float2 tex : TEXCOORD0; };
    static const float k1 = 0.22f; static const float k2 = 0.24f; static const float IPD_SHIFT = 0.05f;
    float2 ApplyDistortion(float2 uv, float2 center) {
        float2 r = uv - center; float r2 = r.x * r.x + r.y * r.y;
        float distortion = 1.0f + (k1 * r2) + (k2 * r2 * r2);
        return center + (r * distortion);
    }
    float4 main(PixelInputType input) : SV_TARGET {
        float2 texCoord = input.tex; bool isLeftEye = texCoord.x < 0.5f;
        float2 localUV, center, distortedUV;
        if (isLeftEye) { localUV = float2(texCoord.x * 2.0f, texCoord.y); center = float2(0.5f + IPD_SHIFT, 0.5f); }
        else { localUV = float2((texCoord.x - 0.5f) * 2.0f, texCoord.y); center = float2(0.5f - IPD_SHIFT, 0.5f); }
        distortedUV = ApplyDistortion(localUV, center);
        if (distortedUV.x < 0.0f || distortedUV.x > 1.0f || distortedUV.y < 0.0f || distortedUV.y > 1.0f) return float4(0.0f, 0.0f, 0.0f, 1.0f);
        return shaderTexture.Sample(SampleType, distortedUV);
    })";

    bool InitializeRenderer(IDXGISwapChain* pSwapChain) {
        Log("InitializeRenderer triggered.");
        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device))) { Log("Failed GetDevice"); return false; }
        g_device->GetImmediateContext(&g_context);

        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        
        D3D11_TEXTURE2D_DESC desc; pBackBuffer->GetDesc(&desc);
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        g_device->CreateTexture2D(&desc, NULL, &g_stereoTexture);
        
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = desc.Format; srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0; srvDesc.Texture2D.MipLevels = 1;
        g_device->CreateShaderResourceView(g_stereoTexture, &srvDesc, &g_copySRV);
        pBackBuffer->Release();

        ID3DBlob* vsBlob = nullptr; 
        if (FAILED(D3DCompile(g_vsCode, strlen(g_vsCode), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &vsBlob, NULL))) { Log("Failed compiling VS"); return false; }
        g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &g_vertexShader); vsBlob->Release();

        ID3DBlob* psBlob = nullptr; 
        if (FAILED(D3DCompile(g_psCode, strlen(g_psCode), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &psBlob, NULL))) { Log("Failed compiling PS"); return false; }
        g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &g_pixelShader); psBlob->Release();

        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP; sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        g_device->CreateSamplerState(&sampDesc, &g_samplerState);

        D3D11_RASTERIZER_DESC rastDesc = {}; rastDesc.FillMode = D3D11_FILL_SOLID; rastDesc.CullMode = D3D11_CULL_NONE;
        g_device->CreateRasterizerState(&rastDesc, &g_rasterState);
        D3D11_DEPTH_STENCIL_DESC dsDesc = {}; dsDesc.DepthEnable = FALSE; dsDesc.StencilEnable = FALSE;
        g_device->CreateDepthStencilState(&dsDesc, &g_depthState);
        D3D11_BLEND_DESC blendDesc = {}; blendDesc.RenderTarget[0].BlendEnable = FALSE; blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        g_device->CreateBlendState(&blendDesc, &g_blendState);

        VRMod::OpenXRLayer::Initialize();
        g_rendererInitialized = true;
        Log("InitializeRenderer SUCCESS.");
        return true;
    }

    void WINAPI hooked_DrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
        if (!g_rendererInitialized) {
            original_DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
            return;
        }

        UINT numViewports = 1;
        D3D11_VIEWPORT vp;
        pContext->RSGetViewports(&numViewports, &vp);

        if (numViewports > 0 && vp.Width > 1000.0f) {
            D3D11_VIEWPORT leftVp = vp; leftVp.Width /= 2.0f;
            D3D11_VIEWPORT rightVp = leftVp; rightVp.TopLeftX += leftVp.Width;

            pContext->RSSetViewports(1, &leftVp);
            original_DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);

            pContext->RSSetViewports(1, &rightVp);
            original_DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);

            pContext->RSSetViewports(1, &vp);
        } else {
            original_DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
        }
    }

    void PerformVRRender(IDXGISwapChain* pSwapChain) {
        if (!g_rendererInitialized) if (!InitializeRenderer(pSwapChain)) return;
        if (!g_context) return;

        // Failsafe: Don't draw the VR split-screen if the headset isn't running,
        // UNLESS the user explicitly forced it in vr_config.ini for debugging!
        if (!VRMod::OpenXRLayer::IsInitialized() && !g_forceSplitScreen) return;

        ID3D11Texture2D* pBackBuffer = nullptr;
        if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) return;
            
        D3D11_TEXTURE2D_DESC bbDesc; pBackBuffer->GetDesc(&bbDesc);
        
        // Resolve MSAA backbuffer if needed
        ID3D11Texture2D* pResolvedBuffer = pBackBuffer;
        if (bbDesc.SampleDesc.Count > 1) {
            // We would resolve it here, but for now we assume non-MSAA or rely on shader
        }
        
        // Create an SRV for the backbuffer to sample from
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = bbDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        if (!g_copySRV) g_device->CreateShaderResourceView(pBackBuffer, &srvDesc, &g_copySRV);

        // Render to the OpenXR g_stereoTexture instead of pBackBuffer!
        ID3D11RenderTargetView* stereoRTV = nullptr;
        g_device->CreateRenderTargetView(g_stereoTexture, NULL, &stereoRTV);

        ID3D11RenderTargetView* oldRTV = nullptr; ID3D11DepthStencilView* oldDSV = nullptr;
        g_context->OMGetRenderTargets(1, &oldRTV, &oldDSV);
        D3D11_PRIMITIVE_TOPOLOGY oldTopology; g_context->IAGetPrimitiveTopology(&oldTopology);
        UINT numViewports = 1; D3D11_VIEWPORT oldVP; g_context->RSGetViewports(&numViewports, &oldVP);
        ID3D11RasterizerState* oldRS = nullptr; g_context->RSGetState(&oldRS);
        ID3D11DepthStencilState* oldDS = nullptr; UINT oldStencilRef; g_context->OMGetDepthStencilState(&oldDS, &oldStencilRef);
        ID3D11BlendState* oldBlend = nullptr; FLOAT oldBlendFactor[4]; UINT oldSampleMask; g_context->OMGetBlendState(&oldBlend, oldBlendFactor, &oldSampleMask);

        g_context->OMSetRenderTargets(1, &stereoRTV, NULL);
        g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_context->IASetInputLayout(NULL);
        D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)bbDesc.Width, (float)bbDesc.Height, 0.0f, 1.0f };
        g_context->RSSetViewports(1, &vp);
        g_context->RSSetState(g_rasterState);
        g_context->OMSetDepthStencilState(g_depthState, 0);
        g_context->OMSetBlendState(g_blendState, NULL, 0xFFFFFFFF);

        g_context->VSSetShader(g_vertexShader, NULL, 0);
        g_context->PSSetShader(g_pixelShader, NULL, 0);
        g_context->PSSetShaderResources(0, 1, &g_copySRV);
        g_context->PSSetSamplers(0, 1, &g_samplerState);
        
        g_context->Draw(3, 0);

        ID3D11ShaderResourceView* nullSRV[1] = { nullptr }; g_context->PSSetShaderResources(0, 1, nullSRV);
        g_context->OMSetRenderTargets(1, &oldRTV, oldDSV);
        g_context->IASetPrimitiveTopology(oldTopology);
        g_context->RSSetViewports(1, &oldVP);
        g_context->RSSetState(oldRS);
        g_context->OMSetDepthStencilState(oldDS, oldStencilRef);
        g_context->OMSetBlendState(oldBlend, oldBlendFactor, oldSampleMask);
            
        if (oldRTV) oldRTV->Release(); if (oldDSV) oldDSV->Release();
        if (oldRS) oldRS->Release(); if (oldDS) oldDS->Release(); if (oldBlend) oldBlend->Release();
        stereoRTV->Release(); pBackBuffer->Release();

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

