#pragma once
#include <d3d12.h>

class RenderTarget
{
public:
    RenderTarget();
    RenderTarget(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    ~RenderTarget();

    ID3D12Resource* resource() { return m_resource; }
    void clear();
    
private:
    ID3D12Resource* m_resource;
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
};

