#pragma once
#include <dxgi1_4.h>
#include <dxgidebug.h>

#include "DrawableCube.h"
#include "Model.h"
#include "../build/src/MeshletizedModel.h"
#include "DX12Wrappers/CommandQueue.h"
#define DX12_ENABLE_DEBUG_LAYER 

class PipelineState;

class Renderer
{
public:
    Renderer();
    ~Renderer() = default;

    static Renderer* get_instance();

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

    static void transition_resource(ID3D12GraphicsCommandList2* commandList, ID3D12Resource* resource,
        D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

    CommandQueue* get_cmd_queue(D3D12_COMMAND_LIST_TYPE type) const;
    ID3D12Resource* get_current_back_buffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE get_current_rtv() const;
    ID3D12DescriptorHeap* get_dsv_heap() const;
    ID3D12DescriptorHeap* get_srv_desc_heap() const { return g_pd3dSrvDescHeap; }

    static constexpr int NUM_BACK_BUFFERS = 3;
    static constexpr int NUM_FRAMES_IN_FLIGHT = 3;

    ID3D12GraphicsCommandList6* g_pd3dCommandList = nullptr;
    void create_depth_stencil();
    void on_window_resize();

    Entity* camera_entity;
    UINT8* m_cbvDataBegin;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    int frame_index = 0;

private:

    // Create
    bool create_device_d3d(HWND hWnd);
    void create_render_targets();

    // Cleanup
    void cleanup_device_d3d();
    void cleanup_render_targets();

    // Clear resources
    void clear_rtv(ID3D12GraphicsCommandList2* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);
    void clear_depth(ID3D12GraphicsCommandList2* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 0.1f);

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

    // Previously used ID3D12Debug6, but it's available in Agility SDK 1.7+ (which for now is in preview)
    // For multiplatform use, let's downgrade it to ID3D12Debug3
	ID3D12Debug3* pdx12Debug = nullptr;
	IDXGIDebug1* pDXGIDebug = nullptr;
    ID3D12InfoQueue* pInfoQueue = nullptr;

    DrawableCube* cube;
    Model* model;
    MeshletizedModel* meshletizedModel;


    static Renderer* m_instance;
};

