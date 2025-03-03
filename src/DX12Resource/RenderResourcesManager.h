#pragma once
#include <vector>

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
    RenderTarget* getCurrentBackbufferRenderTarget();

    D3D12_CPU_DESCRIPTOR_HANDLE getDSVHandle();



private:
    RenderTarget* m_mainRenderTarget[3] = {};
    ID3D12DescriptorHeap* m_mainRTVDescriptorHeap = nullptr;

    DepthStencil* m_mainDepthStencil = nullptr;
};

