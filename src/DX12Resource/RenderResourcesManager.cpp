#include "RenderResourcesManager.h"

#include "Renderer.h"
#include "RenderTarget.h"
#include "utils/ErrorHandler.h"


void RenderResourcesManager::createResources()
{
    // Create main render targets
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.NumDescriptors = 3;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 1;

    auto device = Renderer::get_instance()->get_device();
    AssertFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_mainRTVDescriptorHeap)));

    SIZE_T rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_mainRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    ID3D12Resource* backBuffers[3] = {};
    for (int i = 0; i < 3; i++)
    {
        ID3D12Resource* nBackBuffer = nullptr;
        Renderer::get_instance()->getSwapChain()->GetBuffer(i, IID_PPV_ARGS(&nBackBuffer));
        backBuffers[i] = nBackBuffer;
    }
    m_mainRenderTarget = new RenderTarget(backBuffers, rtvHandle, rtvDescriptorSize);

    // Create depth stencil
    m_mainDepthStencil = new DepthStencil();

}

void RenderResourcesManager::releaseResources()
{
    m_mainRTVDescriptorHeap->Release();

    delete m_mainRenderTarget;
    delete m_mainDepthStencil;
}

void RenderResourcesManager::clearRenderTargets()
{
    auto swapChain = Renderer::get_instance()->getSwapChain();
    m_mainRenderTarget->clear();
    m_mainDepthStencil->clear();
}


D3D12_CPU_DESCRIPTOR_HANDLE RenderResourcesManager::getCurrentRTV()
{
    auto device = Renderer::get_instance()->get_device();
    auto swapChain = Renderer::get_instance()->getSwapChain();
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_mainRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        swapChain->GetCurrentBackBufferIndex(), device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
}


D3D12_CPU_DESCRIPTOR_HANDLE RenderResourcesManager::getDSVHandle()
{
    return m_mainDepthStencil->heap()->GetCPUDescriptorHandleForHeapStart();
}
