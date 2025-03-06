#pragma once
#include <d3d12.h>

#include "DX12Wrappers/Resource.h"

class RenderTarget
{
public:
    RenderTarget();
    RenderTarget(ID3D12Resource* resource[3], D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, SIZE_T rtvDescSize);
    ~RenderTarget();

    void setResourceStateToRenderTarget();
    void setResourceStateToPresent();
    D3D12_CPU_DESCRIPTOR_HANDLE* getHandle();

    void clear();
    
private:
    Resource* m_resources[3] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
    SIZE_T m_rtvDescSize;
};

