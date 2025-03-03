#pragma once
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"

class Resource
{
public:
    Resource() = default;
    Resource(ID3D12Resource* dx12Resource);
    ~Resource();


    void transitionResource(D3D12_RESOURCE_STATES newState);
    void create(uint64_t size, void* data);

    ID3D12Resource* getDx12Resource() { return m_dx12Resource; }

    D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress() const { return m_dx12Resource->GetGPUVirtualAddress(); }

private:
    ID3D12Resource* m_dx12Resource;
    D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;

};

