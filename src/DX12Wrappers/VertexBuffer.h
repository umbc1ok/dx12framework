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
    D3D12_VERTEX_BUFFER_VIEW* get_view() { return m_VertexBufferView; }
    void update();
private:
    ID3D12Resource* m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW* m_VertexBufferView;
    u32 m_verticies_count;
    Vertex const* m_data;
};

