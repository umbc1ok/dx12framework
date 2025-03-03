#include "RenderTarget.h"

#include "Renderer.h"

RenderTarget::RenderTarget()
{

}

RenderTarget::RenderTarget(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    m_resource = new Resource(resource);
    m_rtvHandle = rtvHandle;
    Renderer::get_instance()->get_device()->CreateRenderTargetView(resource, nullptr, rtvHandle);
}

RenderTarget::~RenderTarget()
{
    delete m_resource;
}

void RenderTarget::setResourceStateToRenderTarget()
{
    m_resource->transitionResource(D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void RenderTarget::setResourceStateToPresent()
{
    m_resource->transitionResource(D3D12_RESOURCE_STATE_PRESENT);
}


void RenderTarget::clear()
{
    FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

    Renderer::get_instance()->g_pd3dCommandList->ClearRenderTargetView(m_rtvHandle, clearColor, 0, nullptr);
}

