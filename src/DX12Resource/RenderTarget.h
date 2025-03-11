#pragma once
#include <d3d12.h>

#include "DX12Wrappers/Resource.h"

class RenderTarget
{
public:
    RenderTarget(DXGI_FORMAT format, bool triplebuffered);
    RenderTarget(ID3D12Resource* resource[3], D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, SIZE_T rtvDescSize);
    ~RenderTarget();

    void setResourceStateToRenderTarget();
    void setResourceStateToPresent();
    D3D12_CPU_DESCRIPTOR_HANDLE* getHandle();
    Resource* resource();

    void bind(PipelineState* pso, std::string name);

    void clear();
    
private:
    Resource* m_resources[3] = {};
    ID3D12DescriptorHeap* m_rtvHeap = nullptr;
    ID3D12DescriptorHeap* m_srvHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
    SIZE_T m_rtvDescSize;
    SIZE_T m_srvDescSize;


    bool m_tripleBuffered = true;
};

