#pragma once
#include <d3d12.h>

#include "utils/Types.h"

class IndexBuffer
{
public:

    IndexBuffer(u16 const* data, u16 const no_of_indicies);
    ~IndexBuffer();

    ID3D12Resource* get() const;
    void update();
    D3D12_INDEX_BUFFER_VIEW* get_view() { return &m_IndexBufferView; }
private:
    ID3D12Resource* m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};

    u16 const* m_data;
    u16 m_no_of_indicies;
};

