#pragma once
#include <d3d12.h>

#include "DX12Wrappers/Resource.h"

class RenderTarget
{
public:
    RenderTarget();
    RenderTarget(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    ~RenderTarget();

    void setResourceStateToRenderTarget();
    void setResourceStateToPresent();

    Resource* resource() { return m_resource; }
    void clear();
    
private:
    Resource* m_resource;
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
};

