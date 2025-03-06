#include "RenderTarget.h"

#include "Renderer.h"

RenderTarget::RenderTarget()
{

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
    auto index = Renderer::get_instance()->getSwapChain()->GetCurrentBackBufferIndex();
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
    handle.ptr += Renderer::get_instance()->getSwapChain()->GetCurrentBackBufferIndex() * m_rtvDescSize;
    return &handle;
}


void RenderTarget::clear()
{
    FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
    auto rtvHandle = m_rtvHandle;
    rtvHandle.ptr += Renderer::get_instance()->getSwapChain()->GetCurrentBackBufferIndex() * m_rtvDescSize;
    Renderer::get_instance()->g_pd3dCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

