#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "DX12Hook.hpp"
#include <MinHook.h>
#include <iostream>

namespace VRMod {

    static ID3D12CommandQueue* g_commandQueue = nullptr;
    static ID3D12Resource* g_stereoTexture = nullptr;

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
        // TODO: Copy backbuffer to g_stereoTexture using DX12 Command Lists
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

} // namespace VRMod
