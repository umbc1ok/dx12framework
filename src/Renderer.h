#pragma once
#include <dxgi1_4.h>

#include "DrawableCube.h"
#include "DX12Wrappers/CommandQueue.h"


class PipelineState;

class Renderer
{
public:
    Renderer();
    ~Renderer() = default;
    static void create();
    void start_frame();
    void end_frame();
    void cleanup();
    void render();

    ID3D12Device2* get_device() const;

    void update_buffer_resource(ID3D12GraphicsCommandList2* commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    static Renderer* get_instance();
    static void TransitionResource(ID3D12GraphicsCommandList2* commandList, ID3D12Resource* resource,
        D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
    void ClearRTV(ID3D12GraphicsCommandList2* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);
    void ClearDepth(ID3D12GraphicsCommandList2* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 0.1f);
    CommandQueue* get_cmd_queue(D3D12_COMMAND_LIST_TYPE type) const;
    ID3D12Resource* get_current_back_buffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE get_current_rtv() const;
    ID3D12DescriptorHeap* get_dsv_heap() const;
    ID3D12DescriptorHeap* get_srv_desc_heap() { return g_pd3dSrvDescHeap; }

    static int const NUM_BACK_BUFFERS = 3;
    static int const NUM_FRAMES_IN_FLIGHT = 3;

    ID3D12GraphicsCommandList2* g_pd3dCommandList = nullptr;

private:

    // Create
    bool create_device_d3d(HWND hWnd);
    void create_render_targets();

    // Cleanup
    void cleanup_device_d3d();
    void cleanup_render_targets();

    // Basic DX12 stuff
    ID3D12Device2* g_pd3dDevice = nullptr;
    IDXGISwapChain3* g_pSwapChain = nullptr;
    D3D12_VIEWPORT m_Viewport = {};
    D3D12_RECT m_ScissorRect;
    uint64_t* g_fencevalues = new uint64_t[NUM_FRAMES_IN_FLIGHT];

    // Resources
    ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
    ID3D12Resource* m_depth_buffer = nullptr;


    // Descriptor heaps
    ID3D12DescriptorHeap* m_dsv_heap = nullptr;
    ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
    ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;

    // Handles
    D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

    // Command queues
    CommandQueue* m_DirectCommandQueue;
    CommandQueue* m_ComputeCommandQueue;
    CommandQueue* m_CopyCommandQueue;


    PipelineState* m_pipeline_state;


    DrawableCube* cube;

    static Renderer* m_instance;
};

