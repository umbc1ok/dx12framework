#pragma once
#include <d3d12.h>

#include "utils/Types.h"

class IndexBuffer
{
public:

    IndexBuffer(uint32_t const* data, uint32_t const no_of_indicies);
    ~IndexBuffer();

    ID3D12Resource* get() const;
    void update();
    D3D12_INDEX_BUFFER_VIEW* get_view() { return &m_IndexBufferView; }
    uint32_t size() const { return m_no_of_indicies; }

private:
    ID3D12Resource* m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};

    uint32_t const* m_data;
    uint32_t m_no_of_indicies;
};

