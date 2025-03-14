#include "Renderer.h"

#include <dxgidebug.h>

#include "PipelineState.h"
#include "Window.h"
#include "utils/ErrorHandler.h"
#include "utils/Utils.h"
#include <Camera.h>

#include "Input.h"
#include "Keyboard.h"
#include "Tools/GPUProfiler.h"
#include "DX12Resource/RenderTarget.h"
#include "RenderTaskList.h"
Renderer* Renderer::m_instance;

Renderer::Renderer()
{
    create_device_d3d(Window::get_hwnd());
}

void Renderer::create()
{
    m_instance = new Renderer();
    m_instance->m_render_resources_manager = new RenderResourcesManager();
    m_instance->m_render_resources_manager->createResources();

    m_instance->m_render_task_list = new RenderTaskList();
    m_instance->m_render_task_list->prepareMainList();
    // HACK: ImGui was blurry until first window resize
    // This gets rid of that problem, I dont know why
    m_instance->on_window_resize();
    RECT rect;
    int width, height;
    if (GetWindowRect(Window::get_hwnd(), &rect))
    {
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }
    GetWindowRect(Window::get_hwnd(), &rect);
    m_instance->m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
        static_cast<float>(width), static_cast<float>(height));
    m_instance->m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    m_instance->m_debugDrawer = new DebugDrawer();
    GPUProfiler::getInstance()->startRecording();
    //
}

void Renderer::start_frame()
{
    auto kb = Input::getInstance()->m_keyboard->GetState();
    if (kb.F5)
    {
        for(auto& pipelineState: mRegisteredPipelineStates)
        {
            pipelineState->reload();
        }
    }


    auto index = g_pSwapChain->GetCurrentBackBufferIndex();
    frame_index = index;
    auto cmdqueue = get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto command_list = cmdqueue->get_command_list();
	g_pd3dCommandList = command_list;

    auto renderTarget = m_render_resources_manager->getMainRenderTarget();
    renderTarget->setResourceStateToRenderTarget();

}

void Renderer::render()
{
    auto cmd_list = g_pd3dCommandList;
    auto profiler = GPUProfiler::getInstance();
    profiler->startFrame();

    ProfilerEntry* const profilerEntry = profiler->startEntry(cmd_list, "Frame");
    {
        cmd_list->RSSetViewports(1, &m_Viewport);
        cmd_list->RSSetScissorRects(1, &m_ScissorRect);

        m_render_task_list->renderMainList();

        ProfilerEntry* const profilerEntryDrawDebug = profiler->startEntry(cmd_list, "Draw debug geometry");
        {
            m_debugDrawer->draw();
        } profiler->endEntry(cmd_list, profilerEntryDrawDebug);

    } profiler->endEntry(cmd_list, profilerEntry);

}

void Renderer::initDebugDrawings()
{
    m_debugDrawer->createCamera();
}

void Renderer::end_frame()
{
    auto command_list = g_pd3dCommandList;
    auto profiler = GPUProfiler::getInstance();

    auto renderTarget = m_render_resources_manager->getMainRenderTarget();
    renderTarget->setResourceStateToPresent();


    profiler->endRecording(command_list);
    auto index = g_pSwapChain->GetCurrentBackBufferIndex();
    g_fencevalues[g_pSwapChain->GetCurrentBackBufferIndex()] = m_DirectCommandQueue->execute_command_list(command_list);
    m_DirectCommandQueue->wait_for_fence_value(g_fencevalues[index]);

    HRESULT hr = g_pSwapChain->Present(m_vsync, 0); // Present without vsync (set first parameter to 1 to enable
    AssertFailed(hr);
}

void Renderer::cleanup()
{
    cleanup_device_d3d();
}

void Renderer::create_depth_stencil()
{
}

void Renderer::on_window_resize()
{
    RECT rect;
    int width, height;
    if (GetWindowRect(Window::get_hwnd(), &rect))
    {
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }

	m_DirectCommandQueue->flush();
    m_render_resources_manager->releaseResources();


    HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(width), (UINT)HIWORD(height), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
    assert(SUCCEEDED(result) && "Failed to resize swapchain.");
    m_render_resources_manager->createResources();

    m_render_task_list->prepareMainList();
    m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
        static_cast<float>(width), static_cast<float>(height));
}

void Renderer::set_wireframe(const bool& wireframe)
{
    for (auto pipeline_state : mRegisteredPipelineStates)
    {
        pipeline_state->setWireframe(wireframe);
    }
}

void Renderer::register_pipeline_state(PipelineState* const pipeline_state)
{
    mRegisteredPipelineStates.push_back(pipeline_state);
}

bool Renderer::create_device_d3d(HWND hWnd)
{
    RECT rect;
    int width, height;
    if (GetWindowRect(Window::get_hwnd(), &rect))
    {
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }

    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
    {
        pdx12Debug->EnableDebugLayer();
    }

    if(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDXGIDebug))))
    {
		pDXGIDebug->EnableLeakTrackingForThread();
    }
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HRESULT hr = g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap));
        AssertFailed(hr);
    }

    {
        if (g_pd3dDevice)
        {
            m_DirectCommandQueue = new CommandQueue(g_pd3dDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
            m_ComputeCommandQueue = new CommandQueue(g_pd3dDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
            m_CopyCommandQueue = new CommandQueue(g_pd3dDevice, D3D12_COMMAND_LIST_TYPE_COPY);
        }
    }

    {
        IDXGIFactory4* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;

        if (dxgiFactory->CreateSwapChainForHwnd(m_DirectCommandQueue->get_d_3d12_command_queue(), hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
            return false;
        swapChain1->Release();
        dxgiFactory->Release();
        g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
        //g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }
    return true;
}

void Renderer::cleanup_device_d3d()
{
    // Command queues
    delete m_DirectCommandQueue;
    delete m_ComputeCommandQueue;
    delete m_CopyCommandQueue;

    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }

    // Heaps
	if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (m_dsv_heap) { m_dsv_heap->Release(); m_dsv_heap = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pDXGIDebug != nullptr)
    {
        // TODO: TURN THIS ON AND CLEANUP ALL NON-RELEASED OBJECTS
        // Also, does it really matter?
        //pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDXGIDebug->DisableLeakTrackingForThread();
        pDXGIDebug->Release();
    }
#endif
}

ID3D12Device2* Renderer::get_device() const
{
    return g_pd3dDevice;
}

void Renderer::update_buffer_resource(ID3D12GraphicsCommandList2* commandList,
    ID3D12Resource** pDestinationResource,
    ID3D12Resource** pIntermediateResource,
    size_t numElements, size_t elementSize, const void* bufferData,
    D3D12_RESOURCE_FLAGS flags)
{

    size_t bufferSize = numElements * elementSize;

    // Create a committed resource for the GPU resource in a default heap.
    auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    AssertFailed(g_pd3dDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(pDestinationResource)));

    // Create a committed resource for the upload.
    if (bufferData)
    {
        auto upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto buffer_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        AssertFailed(g_pd3dDevice->CreateCommittedResource(
            &upload_heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &buffer_resource_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(pIntermediateResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        UpdateSubresources(commandList,
            *pDestinationResource, *pIntermediateResource,
            0, 0, 1, &subresourceData);
    }
}

Renderer* Renderer::get_instance()
{
    return m_instance;
}


CommandQueue* Renderer::get_cmd_queue(D3D12_COMMAND_LIST_TYPE type) const
{
    CommandQueue* command_queue;
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        command_queue = m_DirectCommandQueue;
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        command_queue = m_ComputeCommandQueue;
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        command_queue = m_CopyCommandQueue;
        break;
    default:
        assert(false && "Invalid command queue type.");
    }

    return command_queue;
}


