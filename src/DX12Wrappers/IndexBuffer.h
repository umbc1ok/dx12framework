#pragma once
#include <d3d12.h>

#include "utils/Types.h"

class IndexBuffer
{
public:

    IndexBuffer(u32 const* data, u32 const no_of_indicies);
    ~IndexBuffer();

    ID3D12Resource* get() const;

private:
    ID3D12Resource* m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
};

