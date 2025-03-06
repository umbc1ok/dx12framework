#pragma once

#include "DepthStencil.h"
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"
class RenderTarget;
class IRenderTask;

class RenderResourcesManager
{
public:
    RenderResourcesManager() = default;
    ~RenderResourcesManager() = default;

    void createResources();
    void releaseResources();
    void clearRenderTargets();


    D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV();
    D3D12_CPU_DESCRIPTOR_HANDLE getDSVHandle();

    /*
     * Returns the pointer to the current Main Render Target
     * The pointer is updated every frame
     */
    RenderTarget* getMainRenderTarget() { return m_mainRenderTarget; }
    DepthStencil* getMainDepthStencil() { return m_mainDepthStencil; }



private:
    ID3D12DescriptorHeap* m_mainRTVDescriptorHeap = nullptr;
    RenderTarget* m_mainRenderTarget = nullptr;
    DepthStencil* m_mainDepthStencil = nullptr;
};

