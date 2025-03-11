#include "RenderTarget.h"

#include "Renderer.h"
#include "Window.h"
#include "utils/ErrorHandler.h"

RenderTarget::RenderTarget(DXGI_FORMAT format, bool triplebuffered)
{
    m_tripleBuffered = triplebuffered;
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 3;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 1;

    auto device = Renderer::get_instance()->get_device();
    AssertFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = triplebuffered ? 3 : 1; // Number of descriptors (1 SRV in this case)
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    AssertFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));


    m_rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    m_srvHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < (triplebuffered ? 3 : 1); i++)
    {
        uint32_t width, height;
        RECT rect;
        if (GetWindowRect(Window::get_hwnd(), &rect))
        {
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
        }
        m_resources[i] = new Resource();

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Alignment = 0;
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        
        m_resources[i]->createTexture(textureDesc);
        auto tempHandle = m_rtvHandle;
        tempHandle.ptr += i * Renderer::get_instance()->get_device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device->CreateRenderTargetView(m_resources[i]->getDx12Resource(), nullptr, tempHandle);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        auto handleCopy = m_srvHandle;
        handleCopy.ptr = m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + i * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        device->CreateShaderResourceView(m_resources[i]->getDx12Resource(), &srvDesc, handleCopy);
    }
    m_rtvDescSize = Renderer::get_instance()->get_device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_srvDescSize = Renderer::get_instance()->get_device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

}

RenderTarget::RenderTarget(ID3D12Resource* resource[3], D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, SIZE_T rtvDescSize)
{
    m_resources[0] = new Resource(resource[0]);
    m_resources[1] = new Resource(resource[1]);
    m_resources[2] = new Resource(resource[2]);
    m_rtvHandle = rtvHandle;
    auto somertvhandle = rtvHandle;
    m_rtvDescSize = rtvDescSize;
    Renderer::get_instance()->get_device()->CreateRenderTargetView(m_resources[0]->getDx12Resource(), nullptr, somertvhandle);
    somertvhandle.ptr += rtvDescSize;
    Renderer::get_instance()->get_device()->CreateRenderTargetView(m_resources[1]->getDx12Resource(), nullptr, somertvhandle);
    somertvhandle.ptr += rtvDescSize;
    Renderer::get_instance()->get_device()->CreateRenderTargetView(m_resources[2]->getDx12Resource(), nullptr, somertvhandle);
}

RenderTarget::~RenderTarget()
{
    delete m_resources[0];
    delete m_resources[1];
    delete m_resources[2];
}

void RenderTarget::setResourceStateToRenderTarget()
{
    auto index = m_tripleBuffered ? Renderer::get_instance()->getSwapChain()->GetCurrentBackBufferIndex() : 0;
    m_resources[index]->transitionResource(D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void RenderTarget::setResourceStateToPresent()
{
    auto index = Renderer::get_instance()->getSwapChain()->GetCurrentBackBufferIndex();
    m_resources[index]->transitionResource(D3D12_RESOURCE_STATE_PRESENT);
}

D3D12_CPU_DESCRIPTOR_HANDLE* RenderTarget::getHandle()
{
    auto handle = m_rtvHandle;
    if (m_tripleBuffered)
        handle.ptr += Renderer::get_instance()->getSwapChain()->GetCurrentBackBufferIndex() * m_rtvDescSize;
    return &handle;
}

Resource* RenderTarget::resource()
{
    auto index = Renderer::get_instance()->getSwapChain()->GetCurrentBackBufferIndex();
    return m_resources[index];
}

void RenderTarget::bind(PipelineState* pso, std::string name)
{
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart());
    if (m_tripleBuffered)
        gpuHandle.ptr += m_srvDescSize * Renderer::get_instance()->getSwapChain()->GetCurrentBackBufferIndex();

    auto cmdList = Renderer::get_instance()->g_pd3dCommandList;
    cmdList->SetDescriptorHeaps(1, &m_srvHeap);
    cmdList->SetGraphicsRootDescriptorTable(pso->getRootParameterIndex(name), gpuHandle);
}


void RenderTarget::clear()
{
    FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
    auto rtvHandle = m_rtvHandle;
    if (m_tripleBuffered)
        rtvHandle.ptr += Renderer::get_instance()->getSwapChain()->GetCurrentBackBufferIndex() * m_rtvDescSize;
    Renderer::get_instance()->g_pd3dCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

