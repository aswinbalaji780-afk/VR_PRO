#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <fstream>
#include <string>
#include "MinHook.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Type definition for IDXGISwapChain::Present
typedef HRESULT(WINAPI* PFN_Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
PFN_Present original_Present = nullptr;

// Custom Rendering State
bool g_rendererInitialized = false;
ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
ID3D11Texture2D* g_copyTexture = nullptr;
ID3D11ShaderResourceView* g_copySRV = nullptr;
ID3D11VertexShader* g_vertexShader = nullptr;
ID3D11PixelShader* g_pixelShader = nullptr;
ID3D11SamplerState* g_samplerState = nullptr;

// The full screen quad vertex shader (generates vertices without a buffer!)
const char* g_vsCode = R"(
struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};
VS_OUTPUT main(uint id : SV_VertexID) {
    VS_OUTPUT output;
    output.tex = float2((id << 1) & 2, id & 2);
    output.pos = float4(output.tex * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}
)";

// The VR stereoscopic pixel shader with lens distortion
const char* g_psCode = R"(
Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);
struct PixelInputType {
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};
static const float k1 = 0.22f;
static const float k2 = 0.24f;
static const float IPD_SHIFT = 0.05f;
float2 ApplyDistortion(float2 uv, float2 center) {
    float2 r = uv - center;
    float r2 = r.x * r.x + r.y * r.y;
    float distortion = 1.0f + (k1 * r2) + (k2 * r2 * r2);
    return center + (r * distortion);
}
float4 main(PixelInputType input) : SV_TARGET {
    float2 texCoord = input.tex;
    bool isLeftEye = texCoord.x < 0.5f;
    float2 localUV, center, distortedUV;
    
    if (isLeftEye) {
        localUV = float2(texCoord.x * 2.0f, texCoord.y);
        center = float2(0.5f + IPD_SHIFT, 0.5f);
    } else {
        localUV = float2((texCoord.x - 0.5f) * 2.0f, texCoord.y);
        center = float2(0.5f - IPD_SHIFT, 0.5f);
    }
    distortedUV = ApplyDistortion(localUV, center);
    if (distortedUV.x < 0.0f || distortedUV.x > 1.0f || distortedUV.y < 0.0f || distortedUV.y > 1.0f) {
        return float4(0.0f, 0.0f, 0.0f, 1.0f); // Vignette
    }
    return shaderTexture.Sample(SampleType, distortedUV);
}
)";

// Simple logger
void Log(const std::string& message) {
    std::ofstream logFile("dx11_vr_hook_log.txt", std::ios_base::app);
    if (logFile.is_open()) logFile << message << "\n";
}

bool InitializeRenderer(IDXGISwapChain* pSwapChain) {
    if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device))) return false;
    g_device->GetImmediateContext(&g_context);

    // Get backbuffer
    ID3D11Texture2D* pBackBuffer = nullptr;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (!pBackBuffer) return false;

    // Create main RTV
    g_device->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);

    // Create texture copy for sampling
    D3D11_TEXTURE2D_DESC desc;
    pBackBuffer->GetDesc(&desc);
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    g_device->CreateTexture2D(&desc, NULL, &g_copyTexture);
    
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    g_device->CreateShaderResourceView(g_copyTexture, &srvDesc, &g_copySRV);

    pBackBuffer->Release();

    // Compile & Create Vertex Shader
    ID3DBlob* vsBlob = nullptr;
    D3DCompile(g_vsCode, strlen(g_vsCode), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &vsBlob, NULL);
    g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &g_vertexShader);
    vsBlob->Release();

    // Compile & Create Pixel Shader
    ID3DBlob* psBlob = nullptr;
    D3DCompile(g_psCode, strlen(g_psCode), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &psBlob, NULL);
    g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &g_pixelShader);
    psBlob->Release();

    // Create Sampler State
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    g_device->CreateSamplerState(&sampDesc, &g_samplerState);

    g_rendererInitialized = true;
    Log("VR Renderer Initialized successfully!");
    return true;
}

// Our custom hooked Present function
HRESULT WINAPI hooked_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_rendererInitialized) {
        InitializeRenderer(pSwapChain);
    }

    if (g_rendererInitialized && g_context) {
        // 1. Copy the game's final rendered frame to our copy texture
        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        g_context->CopyResource(g_copyTexture, pBackBuffer);
        pBackBuffer->Release();

        // 2. State Saving (Minimal)
        ID3D11RenderTargetView* oldRTV = nullptr;
        ID3D11DepthStencilView* oldDSV = nullptr;
        g_context->OMGetRenderTargets(1, &oldRTV, &oldDSV);
        D3D11_PRIMITIVE_TOPOLOGY oldTopology;
        g_context->IAGetPrimitiveTopology(&oldTopology);

        // 3. Set our custom render state
        g_context->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_context->IASetInputLayout(NULL); // Shader uses SV_VertexID, no input layout needed
        
        g_context->VSSetShader(g_vertexShader, NULL, 0);
        g_context->PSSetShader(g_pixelShader, NULL, 0);
        g_context->PSSetShaderResources(0, 1, &g_copySRV);
        g_context->PSSetSamplers(0, 1, &g_samplerState);

        // 4. Draw the full screen quad! (3 vertices generated by SV_VertexID)
        g_context->Draw(3, 0);

        // 5. State Restoring
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        g_context->PSSetShaderResources(0, 1, nullSRV);
        g_context->OMSetRenderTargets(1, &oldRTV, oldDSV);
        g_context->IASetPrimitiveTopology(oldTopology);
        
        if (oldRTV) oldRTV->Release();
        if (oldDSV) oldDSV->Release();
    }

    // Call the original Present function to flip to the monitor
    return original_Present(pSwapChain, SyncInterval, Flags);
}

// Initialization thread (same as before)
DWORD WINAPI InitializeHookThread(LPVOID lpParam) {
    Log("--- MinimalDX11Hook Initializing ---");

    WNDCLASSEX windowClass;
    ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.lpszClassName = "DummyWindowDX11";
    RegisterClassEx(&windowClass);
    HWND dummyWindow = CreateWindow(windowClass.lpszClassName, "Dummy", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

    HMODULE libD3D11 = LoadLibraryA("d3d11.dll");
    typedef HRESULT(WINAPI* PFN_D3D11CreateDeviceAndSwapChain)(
        IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, 
        UINT, UINT, const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, 
        ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
        
    PFN_D3D11CreateDeviceAndSwapChain D3D11CreateDeviceAndSwapChain = 
        (PFN_D3D11CreateDeviceAndSwapChain)GetProcAddress(libD3D11, "D3D11CreateDeviceAndSwapChain");

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.Width = 800;
    swapChainDesc.BufferDesc.Height = 600;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = dummyWindow;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    ID3D11Device* dummyDevice = nullptr;
    IDXGISwapChain* dummySwapChain = nullptr;
    ID3D11DeviceContext* dummyContext = nullptr;
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, 
        &swapChainDesc, &dummySwapChain, &dummyDevice, &featureLevel, &dummyContext);

    void** swapChainVTable = *reinterpret_cast<void***>(dummySwapChain);
    void* presentAddress = swapChainVTable[8];

    dummyDevice->Release();
    dummySwapChain->Release();
    dummyContext->Release();
    DestroyWindow(dummyWindow);

    if (MH_Initialize() != MH_OK) return 1;
    if (MH_CreateHook(presentAddress, (LPVOID)&hooked_Present, reinterpret_cast<LPVOID*>(&original_Present)) != MH_OK) return 1;
    if (MH_EnableHook(presentAddress) != MH_OK) return 1;

    Log("SUCCESS! DX11 Hook successfully injected and waiting for Present...");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InitializeHookThread, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        break;
    }
    return TRUE;
}
