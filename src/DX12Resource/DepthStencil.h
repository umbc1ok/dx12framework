#pragma once

#include "d3d12.h"


// For now only using D32, if I ever need anything else, I will add it
class DepthStencil
{
public:
    DepthStencil();
    ~DepthStencil();

    void clear();

    ID3D12DescriptorHeap* heap() { return m_heap; }
private:
    ID3D12DescriptorHeap* m_heap = nullptr;
    ID3D12Resource* m_resource = nullptr;

};

