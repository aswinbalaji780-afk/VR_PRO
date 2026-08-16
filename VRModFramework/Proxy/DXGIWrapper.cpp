#include <windows.h>

// Forward exports to the real system dxgi.dll
// This allows our DLL to act as a seamless drop-in replacement (Proxy) for games.
// The game will load our DLL thinking it's DXGI, and we forward all legitimate
// graphics API calls back to the OS while our DllMain initializes the VR Mod.

#pragma comment(linker, "/export:CreateDXGIFactory=C:\\Windows\\System32\\dxgi.CreateDXGIFactory")
#pragma comment(linker, "/export:CreateDXGIFactory1=C:\\Windows\\System32\\dxgi.CreateDXGIFactory1")
#pragma comment(linker, "/export:CreateDXGIFactory2=C:\\Windows\\System32\\dxgi.CreateDXGIFactory2")
#pragma comment(linker, "/export:DXGIDeclareAdapterRemovalSupport=C:\\Windows\\System32\\dxgi.DXGIDeclareAdapterRemovalSupport")
#pragma comment(linker, "/export:DXGIGetDebugInterface1=C:\\Windows\\System32\\dxgi.DXGIGetDebugInterface1")
