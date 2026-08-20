#include <windows.h>

// Proper dynamic proxy for dxgi.dll
// We dynamically load the real system dxgi.dll and forward calls to it.

typedef HRESULT(WINAPI* CreateDXGIFactory_t)(REFIID riid, void** ppFactory);
typedef HRESULT(WINAPI* CreateDXGIFactory1_t)(REFIID riid, void** ppFactory);
typedef HRESULT(WINAPI* CreateDXGIFactory2_t)(UINT Flags, REFIID riid, void** ppFactory);
typedef HRESULT(WINAPI* DXGIDeclareAdapterRemovalSupport_t)();
typedef HRESULT(WINAPI* DXGIGetDebugInterface1_t)(UINT Flags, REFIID riid, void** pDebug);

static HMODULE g_realDxgi = nullptr;

static void LoadRealDXGI() {
    if (!g_realDxgi) {
        char sysPath[MAX_PATH];
        GetSystemDirectoryA(sysPath, MAX_PATH);
        strcat_s(sysPath, "\\dxgi.dll");
        g_realDxgi = LoadLibraryA(sysPath);
    }
}

extern "C" __declspec(dllexport) HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory) {
    LoadRealDXGI();
    auto func = (CreateDXGIFactory_t)GetProcAddress(g_realDxgi, "CreateDXGIFactory");
    return func ? func(riid, ppFactory) : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory) {
    LoadRealDXGI();
    auto func = (CreateDXGIFactory1_t)GetProcAddress(g_realDxgi, "CreateDXGIFactory1");
    return func ? func(riid, ppFactory) : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, void** ppFactory) {
    LoadRealDXGI();
    auto func = (CreateDXGIFactory2_t)GetProcAddress(g_realDxgi, "CreateDXGIFactory2");
    return func ? func(Flags, riid, ppFactory) : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DXGIDeclareAdapterRemovalSupport() {
    LoadRealDXGI();
    auto func = (DXGIDeclareAdapterRemovalSupport_t)GetProcAddress(g_realDxgi, "DXGIDeclareAdapterRemovalSupport");
    return func ? func() : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, void** pDebug) {
    LoadRealDXGI();
    auto func = (DXGIGetDebugInterface1_t)GetProcAddress(g_realDxgi, "DXGIGetDebugInterface1");
    return func ? func(Flags, riid, pDebug) : E_FAIL;
}
