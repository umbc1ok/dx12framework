#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <string>

struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

class Window
{
public:
    static void create();
    void update();
    void start_frame();
    void end_frame();
    void cleanup();
    static Window* get_instance()
    {
        return m_instance;
    }
    ID3D12Device* get_device();
    ID3D12GraphicsCommandList* get_cmd_list();
    Window() {};
    ~Window() {};
    static int const NUM_BACK_BUFFERS = 3;
    static int const NUM_FRAMES_IN_FLIGHT = 3;
    static HWND get_hwnd() { return hwnd; }
    ID3D12DescriptorHeap* get_srv_desc_heap() { return g_pd3dSrvDescHeap; }
    static void update_window_name(std::string const& name);

private:
    static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool create_device_d3d(HWND hWnd);
    void create_render_targets();
    void cleanup_device_d3d();
    void cleanup_render_targets();
    void wait_for_last_submitted_frame();
    FrameContext* wait_for_next_frame_resources();

    static Window* m_instance;
    static WNDCLASSEXW wc;

    FrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
    FrameContext* frameCtx = nullptr;
    D3D12_RESOURCE_BARRIER barrier = {};
    UINT g_frameIndex = 0;

    ID3D12Device* g_pd3dDevice = nullptr;
    ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
    ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
    ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
    ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
    ID3D12Fence* g_fence = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
    IDXGISwapChain3* g_pSwapChain = nullptr;
    HANDLE g_fenceEvent = nullptr;
    HANDLE g_hSwapChainWaitableObject = nullptr;

    BOOL g_SwapChainOccluded = false;

    ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};

    UINT64 g_fenceLastSignaledValue = 0;

    static HWND hwnd;

};
