#pragma once
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"

class Resource
{
public:
    Resource() = default;
    ~Resource() = default;

    void create(uint64_t size, void* data);
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_dx12Resource->GetGPUVirtualAddress(); }

private:
    ID3D12Resource* m_dx12Resource;

};

