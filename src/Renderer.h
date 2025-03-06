#pragma once
#include <dxgi1_4.h>
#include <dxgidebug.h>

#include "debugGeometry/DebugDrawer.h"
#include "DX12Resource/RenderResourcesManager.h"
#include "DX12Wrappers/CommandQueue.h"
#include "utils/Types.h"
#define DX12_ENABLE_DEBUG_LAYER 

class PipelineState;
class Entity;

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
    void initDebugDrawings();


    ID3D12Device2* get_device() const;

    void update_buffer_resource(ID3D12GraphicsCommandList2* commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    CommandQueue* get_cmd_queue(D3D12_COMMAND_LIST_TYPE type) const;

    ID3D12DescriptorHeap* get_srv_desc_heap() const { return g_pd3dSrvDescHeap; }

    static constexpr int NUM_BACK_BUFFERS = 3;
    static constexpr int NUM_FRAMES_IN_FLIGHT = 3;

    ID3D12GraphicsCommandList6* g_pd3dCommandList = nullptr;
    void create_depth_stencil();
    void on_window_resize();

    Entity* camera_entity;
    int frame_index = 0;

    void set_wireframe(const bool& wireframe);

    void set_debug_mode(const u32& mode) { m_debug_mode = mode; }
    u32 get_debug_mode() const { return m_debug_mode; }
    DebugDrawer* getDebugDrawer() { return m_debugDrawer; }

    void set_vsync(const bool& vsync) { m_vsync = vsync; }
    bool vsync() const { return m_vsync; }
    void register_pipeline_state(PipelineState* pipeline_state);


    IDXGISwapChain3* getSwapChain() const { return g_pSwapChain; }
private:
    RenderResourcesManager* m_render_resources_manager;
    // Create
    bool create_device_d3d(HWND hWnd);

    // Cleanup
    void cleanup_device_d3d();


    // Basic DX12 stuff
    ID3D12Device2* g_pd3dDevice = nullptr;
    IDXGISwapChain3* g_pSwapChain = nullptr;
    D3D12_VIEWPORT m_Viewport = {};
    D3D12_RECT m_ScissorRect;
    uint64_t* g_fencevalues = new uint64_t[NUM_FRAMES_IN_FLIGHT];

    // Resources


    // Descriptor heaps
    ID3D12DescriptorHeap* m_dsv_heap = nullptr;
    ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
    ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;

    // Command queues
    CommandQueue* m_DirectCommandQueue;
    CommandQueue* m_ComputeCommandQueue;
    CommandQueue* m_CopyCommandQueue;


    // Previously used ID3D12Debug6, but it's available in Agility SDK 1.7+ (which for now is in preview)
    // For multiplatform use, let's downgrade it to ID3D12Debug3
	ID3D12Debug3* pdx12Debug = nullptr;
	IDXGIDebug1* pDXGIDebug = nullptr;
    ID3D12InfoQueue* pInfoQueue = nullptr;

    std::vector<PipelineState*> mRegisteredPipelineStates;

    DebugDrawer* m_debugDrawer;

    u32 m_debug_mode = 1;
    bool m_vsync = true;
    static Renderer* m_instance;
};

