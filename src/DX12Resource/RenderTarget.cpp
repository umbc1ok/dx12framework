#include "RenderTarget.h"

#include "Renderer.h"

RenderTarget::RenderTarget()
{

}

RenderTarget::RenderTarget(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    m_resource = resource;
    m_rtvHandle = rtvHandle;
    Renderer::get_instance()->get_device()->CreateRenderTargetView(resource, nullptr, rtvHandle);
}

RenderTarget::~RenderTarget()
{
    m_resource->Release();
}

void RenderTarget::clear()
{
    FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

    Renderer::get_instance()->g_pd3dCommandList->ClearRenderTargetView(m_rtvHandle, clearColor, 0, nullptr);
}

