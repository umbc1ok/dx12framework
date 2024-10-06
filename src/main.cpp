#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxcompiler.lib")

//#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
//#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#define FORCE_DEDICATED_GPU 1

#if FORCE_DEDICATED_GPU
extern "C"
{
    __declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}

extern "C"
{
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#include "Engine.h"

// Main code
int main(int, char**)
{
    Engine::setup();
    Engine::run();
    Engine::cleanup();
}
