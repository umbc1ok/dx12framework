#include "Renderer.h"

#include "DrawableCube.h"
#include "PipelineState.h"
#include "Window.h"
#include "utils/ErrorHandler.h"

Renderer* Renderer::m_instance;

Renderer::Renderer()
{
    create_device_d3d(Window::get_hwnd());
}

void Renderer::create()
{
    m_instance = new Renderer();
    m_instance->m_pipeline_state = new PipelineState("basic.hlsl", "basic.hlsl");
    m_instance->m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
        static_cast<float>(1920), static_cast<float>(1080));
    m_instance->m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    m_instance->cube = new DrawableCube();
}

void Renderer::start_frame()
{
    auto command_list = m_DirectCommandQueue->get_command_list();
    g_pd3dCommandList = command_list;

    Renderer::transition_resource(g_pd3dCommandList, get_current_back_buffer(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

    g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
}

void Renderer::end_frame()
{
    transition_resource(g_pd3dCommandList, get_current_back_buffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    auto index = g_pSwapChain->GetCurrentBackBufferIndex();
    g_fencevalues[g_pSwapChain->GetCurrentBackBufferIndex()] = m_DirectCommandQueue->execute_command_list(g_pd3dCommandList);
    HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync (set first parameter to 1 to enable)

    m_DirectCommandQueue->wait_for_fence_value(g_fencevalues[index]);
}

void Renderer::cleanup()
{
    cleanup_device_d3d();
    cleanup_render_targets();
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
    cleanup_render_targets();
    HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(width), (UINT)HIWORD(height), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
    assert(SUCCEEDED(result) && "Failed to resize swapchain.");
    create_render_targets();
    m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
        static_cast<float>(width), static_cast<float>(height));
}

bool Renderer::create_device_d3d(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = 1920;
        sd.Height = 1080;
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
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
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

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    AssertFailed(g_pd3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsv_heap)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 0.1f, 0 };

    // TODO: Clear those heapype and rsrc_desc
    auto heaptype = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto rsrc_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, 1920, 1080,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    AssertFailed(g_pd3dDevice->CreateCommittedResource(
        &heaptype,
        D3D12_HEAP_FLAG_NONE,
        &rsrc_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&m_depth_buffer)
    ));

    g_pd3dDevice->CreateDepthStencilView(m_depth_buffer, &dsv,
        m_dsv_heap->GetCPUDescriptorHandleForHeapStart());

    create_render_targets();

    return true;
}

void Renderer::create_render_targets()
{
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void Renderer::cleanup_device_d3d()
{
    cleanup_render_targets();
    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void Renderer::cleanup_render_targets()
{
    // PROBABLY SHOULD WAIT FOR LAST FRAME TO FINISH RENDERING
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; }
}


void Renderer::render()
{
    auto window = Window::get_instance();
    auto cmdqueue = get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto cmd_list = g_pd3dCommandList;
    auto back_buffer = get_current_back_buffer();
    auto rtv = get_current_rtv();
    auto dsv = get_dsv_heap()->GetCPUDescriptorHandleForHeapStart();

    FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

    clear_rtv(cmd_list, rtv, clearColor);
    clear_depth(cmd_list, dsv);

    cmd_list->SetPipelineState(m_pipeline_state->get_pipeline_state());
    cmd_list->SetGraphicsRootSignature(m_pipeline_state->get_root_signature());

    cmd_list->RSSetViewports(1, &m_Viewport);
    cmd_list->RSSetScissorRects(1, &m_ScissorRect);

    cmd_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    cube->draw();
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

void Renderer::transition_resource(ID3D12GraphicsCommandList2* commandList,
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource,
        beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}

void Renderer::clear_rtv(ID3D12GraphicsCommandList2* commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void Renderer::clear_depth(ID3D12GraphicsCommandList2* commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
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

ID3D12Resource* Renderer::get_current_back_buffer() const
{
    return g_mainRenderTargetResource[g_pSwapChain->GetCurrentBackBufferIndex()];
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::get_current_rtv() const
{

    return CD3DX12_CPU_DESCRIPTOR_HANDLE(g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pSwapChain->GetCurrentBackBufferIndex(), g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
}

ID3D12DescriptorHeap* Renderer::get_dsv_heap() const
{
    return m_dsv_heap;
}

