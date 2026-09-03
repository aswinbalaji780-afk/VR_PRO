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


    static bool g_forceSplitScreen = true;

    // Globals for D3D11 Hooking
    static bool g_rendererInitialized = false;
    static ID3D11Device* g_device = nullptr;
    static ID3D11DeviceContext* g_context = nullptr;
    static ID3D11Texture2D* g_stereoTexture = nullptr; 
    static ID3D11Texture2D* g_copyTexture = nullptr;
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
    float4 main(PixelInputType input) : SV_TARGET {
        float2 uv = input.tex;
        // Side-by-Side (SBS) split:
        // Left eye (0.0 to 0.5) maps to full (0.0 to 1.0)
        // Right eye (0.5 to 1.0) maps to full (0.0 to 1.0)
        float eyeU = (uv.x < 0.5f) ? (uv.x * 2.0f) : ((uv.x - 0.5f) * 2.0f);
        float eyeV = uv.y;
        
        // Stereoscopic IPD parallax offset between left and right eye
        float ipdOffset = (uv.x < 0.5f) ? -0.008f : 0.008f;
        float2 sampleUV = float2(clamp(eyeU + ipdOffset, 0.0f, 1.0f), eyeV);
        
        return shaderTexture.Sample(SampleType, sampleUV);
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
        
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        g_device->CreateTexture2D(&desc, NULL, &g_copyTexture);
        
        DXGI_FORMAT viewFormat = desc.Format;
        if (viewFormat == DXGI_FORMAT_R8G8B8A8_TYPELESS) viewFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        else if (viewFormat == DXGI_FORMAT_B8G8R8A8_TYPELESS) viewFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        else if (viewFormat == DXGI_FORMAT_R10G10B10A2_TYPELESS) viewFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
        else if (viewFormat == DXGI_FORMAT_R16G16B16A16_TYPELESS) viewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = viewFormat;
        srvDesc.ViewDimension = (desc.SampleDesc.Count > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0; srvDesc.Texture2D.MipLevels = 1;
        g_device->CreateShaderResourceView(g_copyTexture, &srvDesc, &g_copySRV);

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
        original_DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    void RenderSplitScreenToBackBuffer(ID3D11Texture2D* pBackBuffer) {
        if (!pBackBuffer || !g_device || !g_context || !g_vertexShader || !g_pixelShader || !g_copySRV) return;

        D3D11_TEXTURE2D_DESC desc;
        pBackBuffer->GetDesc(&desc);

        ID3D11RenderTargetView* backBufferRTV = nullptr;
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = desc.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        if (FAILED(g_device->CreateRenderTargetView(pBackBuffer, &rtvDesc, &backBufferRTV))) {
            return;
        }

        // Save old pipeline state
        ID3D11RenderTargetView* oldRTV = nullptr;
        ID3D11DepthStencilView* oldDSV = nullptr;
        g_context->OMGetRenderTargets(1, &oldRTV, &oldDSV);
        D3D11_PRIMITIVE_TOPOLOGY oldTopology;
        g_context->IAGetPrimitiveTopology(&oldTopology);
        UINT numViewports = 1;
        D3D11_VIEWPORT oldVP;
        g_context->RSGetViewports(&numViewports, &oldVP);
        ID3D11RasterizerState* oldRS = nullptr;
        g_context->RSGetState(&oldRS);
        ID3D11DepthStencilState* oldDS = nullptr;
        UINT oldStencilRef = 0;
        g_context->OMGetDepthStencilState(&oldDS, &oldStencilRef);
        ID3D11BlendState* oldBlend = nullptr;
        FLOAT oldBlendFactor[4];
        UINT oldSampleMask = 0xFFFFFFFF;
        g_context->OMGetBlendState(&oldBlend, oldBlendFactor, &oldSampleMask);

        // Set Render Target to BackBuffer
        g_context->OMSetRenderTargets(1, &backBufferRTV, NULL);
        g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_context->IASetInputLayout(NULL);

        D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 1.0f };
        g_context->RSSetViewports(1, &vp);
        g_context->RSSetState(g_rasterState);
        g_context->OMSetDepthStencilState(g_depthState, 0);
        g_context->OMSetBlendState(g_blendState, NULL, 0xFFFFFFFF);

        g_context->VSSetShader(g_vertexShader, NULL, 0);
        g_context->PSSetShader(g_pixelShader, NULL, 0);
        g_context->PSSetShaderResources(0, 1, &g_copySRV);
        g_context->PSSetSamplers(0, 1, &g_samplerState);

        // Draw Fullscreen Quad (3 vertices generates full-screen triangle in vertex shader)
        g_context->Draw(3, 0);

        // Unbind SRV to prevent resource hazards
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        g_context->PSSetShaderResources(0, 1, nullSRV);

        // Restore old pipeline state
        g_context->OMSetRenderTargets(1, &oldRTV, oldDSV);
        g_context->IASetPrimitiveTopology(oldTopology);
        g_context->RSSetViewports(1, &oldVP);
        g_context->RSSetState(oldRS);
        g_context->OMSetDepthStencilState(oldDS, oldStencilRef);
        g_context->OMSetBlendState(oldBlend, oldBlendFactor, oldSampleMask);

        if (oldRTV) oldRTV->Release();
        if (oldDSV) oldDSV->Release();
        if (oldRS) oldRS->Release();
        if (oldDS) oldDS->Release();
        if (oldBlend) oldBlend->Release();
        backBufferRTV->Release();
    }

    void PerformVRRender(IDXGISwapChain* pSwapChain) {
        if (!g_rendererInitialized) if (!InitializeRenderer(pSwapChain)) return;
        if (!g_context) return;

        ID3D11Texture2D* pBackBuffer = nullptr;
        if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) return;
            
        // Copy original game backbuffer to our g_copyTexture
        g_context->CopyResource(g_copyTexture, pBackBuffer);
        
        // Render Side-by-Side (SBS) split screen directly on desktop backbuffer
        if (g_forceSplitScreen || !VRMod::OpenXRLayer::IsInitialized()) {
            RenderSplitScreenToBackBuffer(pBackBuffer);
        }

        pBackBuffer->Release();

        // If a VR headset is active, also transmit frame to OpenXR
        if (VRMod::OpenXRLayer::IsInitialized()) {
            VRMod::OpenXRLayer::RenderFrame();
        }
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
        if (GetPrivateProfileStringA("Debug", "ForceSplitScreen", "true", forceBuf, 16, configPath.c_str())) {
            if (_stricmp(forceBuf, "false") == 0) {
                g_forceSplitScreen = false;
                Log("DEBUG: ForceSplitScreen is disabled.");
            } else {
                g_forceSplitScreen = true;
                Log("DEBUG: ForceSplitScreen is enabled.");
            }
        }

        CreateThread(nullptr, 0, InitializeHookThread, nullptr, 0, nullptr);
        return true;
    }
    void DX11HookImpl::Shutdown() { MH_DisableHook(MH_ALL_HOOKS); MH_Uninitialize(); VRMod::OpenXRLayer::Shutdown(); }
    void* DX11HookImpl::GetStereoTexture() {
        return g_copyTexture;
    }

    void* DX11HookImpl::GetDeviceContext() {
        return g_device; // For OpenXR session creation
    }

    void DX11HookImpl::CopyToOpenXRSwapchain(void* destTexture) {
        if (!g_context || !g_copySRV || !destTexture) return;

        ID3D11Texture2D* pDestTex = (ID3D11Texture2D*)destTexture;
        D3D11_TEXTURE2D_DESC desc; pDestTex->GetDesc(&desc);

        ID3D11RenderTargetView* destRTV = nullptr;
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = desc.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        if (FAILED(g_device->CreateRenderTargetView(pDestTex, &rtvDesc, &destRTV))) return;

        ID3D11RenderTargetView* oldRTV = nullptr; ID3D11DepthStencilView* oldDSV = nullptr;
        g_context->OMGetRenderTargets(1, &oldRTV, &oldDSV);
        D3D11_PRIMITIVE_TOPOLOGY oldTopology; g_context->IAGetPrimitiveTopology(&oldTopology);
        UINT numViewports = 1; D3D11_VIEWPORT oldVP; g_context->RSGetViewports(&numViewports, &oldVP);
        ID3D11RasterizerState* oldRS = nullptr; g_context->RSGetState(&oldRS);
        ID3D11DepthStencilState* oldDS = nullptr; UINT oldStencilRef; g_context->OMGetDepthStencilState(&oldDS, &oldStencilRef);
        ID3D11BlendState* oldBlend = nullptr; FLOAT oldBlendFactor[4]; UINT oldSampleMask; g_context->OMGetBlendState(&oldBlend, oldBlendFactor, &oldSampleMask);

        g_context->OMSetRenderTargets(1, &destRTV, NULL);
        g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_context->IASetInputLayout(NULL);
        D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 1.0f };
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
        destRTV->Release();
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

