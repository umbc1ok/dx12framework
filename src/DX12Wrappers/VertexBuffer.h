#pragma once
#include <d3d12.h>

#include "Vertex.h"
#include "utils/Types.h"

class VertexBuffer
{
public:
    VertexBuffer(Vertex const* data, u32 vertices_count);
    ~VertexBuffer();

    ID3D12Resource* get() const;

private:
    ID3D12Resource* m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};

