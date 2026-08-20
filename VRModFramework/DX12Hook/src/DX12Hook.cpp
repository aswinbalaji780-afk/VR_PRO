#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "DX12Hook.hpp"
#include <MinHook.h>
#include <iostream>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace VRMod {

    static ID3D12Device* g_device = nullptr;
    static ID3D12CommandQueue* g_commandQueue = nullptr;
    static ID3D12Resource* g_stereoTexture = nullptr;

    static ID3D12CommandAllocator* g_commandAllocator = nullptr;
    static ID3D12GraphicsCommandList* g_commandList = nullptr;
    static ID3D12RootSignature* g_rootSignature = nullptr;
    static ID3D12PipelineState* g_pso = nullptr;
    static ID3D12DescriptorHeap* g_srvHeap = nullptr;
    static ID3D12DescriptorHeap* g_rtvHeap = nullptr;
    
    static ID3D12Fence* g_fence = nullptr;
    static UINT64 g_fenceValue = 0;
    static HANDLE g_fenceEvent = nullptr;
    static bool g_rendererInitialized = false;

    typedef HRESULT(WINAPI* PFN_Present)(IDXGISwapChain*, UINT, UINT);
    typedef HRESULT(WINAPI* PFN_Present1)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
    
    static PFN_Present original_Present = nullptr;
    static PFN_Present1 original_Present1 = nullptr;

    void PerformVRRenderDX12(IDXGISwapChain* pSwapChain) {
        if (!g_commandQueue) {
            // In DX12, the "device" bound to the swap chain is actually the command queue
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12CommandQueue), (void**)&g_commandQueue))) {
                std::cout << "[DX12Hook] Intercepted Command Queue!\n";
            }
        }
        
        ID3D12Resource* pBackBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&pBackBuffer))) {
            g_stereoTexture = pBackBuffer; // Fallback, updated later by OpenXR copy
            pBackBuffer->Release();
        }
    }

    HRESULT WINAPI hooked_Present_DX12(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        PerformVRRenderDX12(pSwapChain);
        return original_Present(pSwapChain, SyncInterval, Flags);
    }

    HRESULT WINAPI hooked_Present1_DX12(IDXGISwapChain1* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
        PerformVRRenderDX12(pSwapChain);
        return original_Present1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
    }

    DWORD WINAPI InitializeDX12HookThread(LPVOID lpParam) {
        WNDCLASSEX windowClass = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, DefWindowProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DummyWindowDX12", NULL };
        RegisterClassEx(&windowClass);
        HWND dummyWindow = CreateWindow("DummyWindowDX12", "Dummy", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

        HMODULE libDXGI = LoadLibraryA("dxgi.dll");
        HMODULE libD3D12 = LoadLibraryA("d3d12.dll");
        if (!libDXGI || !libD3D12) return 0;

        typedef HRESULT(WINAPI* PFN_CreateDXGIFactory)(REFIID, void**);
        typedef HRESULT(WINAPI* PFN_D3D12CreateDevice)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);

        auto CreateDXGIFactory = (PFN_CreateDXGIFactory)GetProcAddress(libDXGI, "CreateDXGIFactory");
        auto D3D12CreateDevice = (PFN_D3D12CreateDevice)GetProcAddress(libD3D12, "D3D12CreateDevice");

        IDXGIFactory4* factory = nullptr;
        if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory4), (void**)&factory))) return 0;

        ID3D12Device* device = nullptr;
        if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device))) return 0;

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ID3D12CommandQueue* commandQueue = nullptr;
        device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&commandQueue);

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 2; swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; swapChainDesc.OutputWindow = dummyWindow;
        swapChainDesc.SampleDesc.Count = 1; swapChainDesc.Windowed = TRUE; swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        IDXGISwapChain* swapChain = nullptr;
        factory->CreateSwapChain(commandQueue, &swapChainDesc, &swapChain);

        if (swapChain) {
            void** swapChainVTable = *reinterpret_cast<void***>(swapChain);
            void* presentAddress = swapChainVTable[8];
            
            IDXGISwapChain1* swapChain1 = nullptr;
            void* present1Address = nullptr;
            if (SUCCEEDED(swapChain->QueryInterface(__uuidof(IDXGISwapChain1), (void**)&swapChain1))) {
                void** swapChain1VTable = *reinterpret_cast<void***>(swapChain1);
                present1Address = swapChain1VTable[22];
                swapChain1->Release();
            }

            MH_Initialize();
            MH_CreateHook(presentAddress, (LPVOID)&hooked_Present_DX12, reinterpret_cast<LPVOID*>(&original_Present));
            if (present1Address) {
                MH_CreateHook(present1Address, (LPVOID)&hooked_Present1_DX12, reinterpret_cast<LPVOID*>(&original_Present1));
            }
            MH_EnableHook(MH_ALL_HOOKS);
            std::cout << "[DX12Hook] Successfully hooked Present!\n";
            swapChain->Release();
        }

        commandQueue->Release();
        device->Release();
        factory->Release();
        DestroyWindow(dummyWindow);
        return 0;
    }

    bool DX12HookImpl::Initialize(HMODULE hModule) {
        CreateThread(nullptr, 0, InitializeDX12HookThread, nullptr, 0, nullptr);
        return true;
    }

    void DX12HookImpl::Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
    }

    void* DX12HookImpl::GetStereoTexture() {
        return g_stereoTexture;
    }

    void* DX12HookImpl::GetDeviceContext() {
        return g_commandQueue; 
    }

    bool DX12HookImpl::InitializeRendererDX12(ID3D12Device* device) {
        g_device = device;
        if (FAILED(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&g_commandAllocator))) return false;
        if (FAILED(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&g_commandList))) return false;
        g_commandList->Close();

        if (FAILED(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&g_fence))) return false;
        g_fenceValue = 1;
        g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        // Create Descriptor Heaps
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        g_device->CreateDescriptorHeap(&srvHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&g_srvHeap);

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 1;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        g_device->CreateDescriptorHeap(&rtvHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&g_rtvHeap);

        // Create Root Signature
        D3D12_DESCRIPTOR_RANGE range = {};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0;
        range.RegisterSpace = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges = &range;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MaxAnisotropy = 1;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 1;
        rootSigDesc.pParameters = &param;
        rootSigDesc.NumStaticSamplers = 1;
        rootSigDesc.pStaticSamplers = &sampler;
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ID3DBlob* signature = nullptr;
        ID3DBlob* error = nullptr;
        D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        g_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&g_rootSignature);
        if (signature) signature->Release();
        if (error) error->Release();

        // Shaders
        const char* shaderCode = R"(
            Texture2D tex : register(t0);
            SamplerState sam : register(s0);
            struct PSInput { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
            PSInput VSMain(uint id : SV_VertexID) {
                PSInput output;
                output.uv = float2((id << 1) & 2, id & 2);
                output.pos = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);
                return output;
            }
            float4 PSMain(PSInput input) : SV_TARGET {
                float2 uv = input.uv;
                uv.x = fmod(uv.x * 2.0f, 1.0f); // Split screen logic
                return tex.Sample(sam, uv);
            }
        )";

        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, nullptr);
        D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, nullptr);

        // PSO
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = g_rootSignature;
        psoDesc.VS = { reinterpret_cast<UINT8*>(vsBlob->GetBufferPointer()), vsBlob->GetBufferSize() };
        psoDesc.PS = { reinterpret_cast<UINT8*>(psBlob->GetBufferPointer()), psBlob->GetBufferSize() };
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        g_device->CreateGraphicsPipelineState(&psoDesc, __uuidof(ID3D12PipelineState), (void**)&g_pso);

        if (vsBlob) vsBlob->Release();
        if (psBlob) psBlob->Release();

        g_rendererInitialized = true;
        return true;
    }

    void DX12HookImpl::RenderSplitScreen(ID3D12Resource* pBackBuffer, ID3D12Resource* pDestTexture) {
        g_commandAllocator->Reset();
        g_commandList->Reset(g_commandAllocator, g_pso);

        // Transition states
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Transition.pResource = pBackBuffer;
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Transition.pResource = pDestTexture;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON; // Usually from OpenXR
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        g_commandList->ResourceBarrier(2, barriers);

        // Setup SRV
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = pBackBuffer->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        g_device->CreateShaderResourceView(pBackBuffer, &srvDesc, g_srvHeap->GetCPUDescriptorHandleForHeapStart());

        // Setup RTV
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = pDestTexture->GetDesc().Format;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        g_device->CreateRenderTargetView(pDestTexture, &rtvDesc, g_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Bind and Draw
        g_commandList->SetGraphicsRootSignature(g_rootSignature);
        ID3D12DescriptorHeap* heaps[] = { g_srvHeap };
        g_commandList->SetDescriptorHeaps(1, heaps);
        g_commandList->SetGraphicsRootDescriptorTable(0, g_srvHeap->GetGPUDescriptorHandleForHeapStart());

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        g_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)pDestTexture->GetDesc().Width, (float)pDestTexture->GetDesc().Height, 0.0f, 1.0f };
        D3D12_RECT scissor = { 0, 0, (LONG)pDestTexture->GetDesc().Width, (LONG)pDestTexture->GetDesc().Height };
        g_commandList->RSSetViewports(1, &vp);
        g_commandList->RSSetScissorRects(1, &scissor);
        g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_commandList->DrawInstanced(3, 1, 0, 0);

        // Transition back
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
        g_commandList->ResourceBarrier(2, barriers);

        g_commandList->Close();

        ID3D12CommandList* ppCommandLists[] = { g_commandList };
        g_commandQueue->ExecuteCommandLists(1, ppCommandLists);

        const UINT64 fence = g_fenceValue;
        g_commandQueue->Signal(g_fence, fence);
        g_fenceValue++;
        if (g_fence->GetCompletedValue() < fence) {
            g_fence->SetEventOnCompletion(fence, g_fenceEvent);
            WaitForSingleObject(g_fenceEvent, INFINITE);
        }
    }

    void DX12HookImpl::CopyToOpenXRSwapchain(void* destTexture) {
        if (!g_commandQueue || !g_stereoTexture || !destTexture) return;

        if (!g_rendererInitialized) {
            ID3D12Device* device = nullptr;
            g_commandQueue->GetDevice(__uuidof(ID3D12Device), (void**)&device);
            if (device) {
                InitializeRendererDX12(device);
                device->Release();
            }
        }

        if (g_rendererInitialized) {
            RenderSplitScreen(g_stereoTexture, (ID3D12Resource*)destTexture);
        }
    }

} // namespace VRMod
