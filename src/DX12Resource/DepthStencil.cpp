#include "DepthStencil.h"

#include "Window.h"
#include "utils/ErrorHandler.h"

DepthStencil::DepthStencil()
{
    auto device = Renderer::get_instance()->get_device();
    if (m_heap != nullptr)
    {
        m_heap->Release();
        m_heap = nullptr;
    }
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    AssertFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_heap)));


    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };

    if (m_resource != nullptr)
    {
        m_resource->Release();
        m_resource = nullptr;
    }

    RECT rect;
    int width, height;
    if (GetWindowRect(Window::get_hwnd(), &rect))
    {
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }

    auto heaptype = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto rsrc_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    AssertFailed(device->CreateCommittedResource(
        &heaptype,
        D3D12_HEAP_FLAG_NONE,
        &rsrc_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&m_resource)
    ));

    device->CreateDepthStencilView(m_resource, &dsv,
        m_heap->GetCPUDescriptorHandleForHeapStart());
}

DepthStencil::~DepthStencil()
{
    m_heap->Release();
    m_resource->Release();
}

void DepthStencil::clear()
{
    Renderer::get_instance()->g_pd3dCommandList->ClearDepthStencilView(m_heap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}
