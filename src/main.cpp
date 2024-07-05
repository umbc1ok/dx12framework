#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

#include "imgui.h"
#include "imgui_impl/imgui_impl_win32.h"
#include "imgui_impl/imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#include "imgui_internal.h"
#include "Engine.h"

// Main code
int main(int, char**)
{
    Engine::setup();
    Engine::run();
    Engine::cleanup();
}
