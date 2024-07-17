#include "VertexBuffer.h"

#include <d3d12.h>

#include "Renderer.h"
#include "Vertex.h"
#include "Window.h"


VertexBuffer::VertexBuffer(Vertex const* data, u32 vertices_count)
{
    size_t no_of_vertices = sizeof(data) / sizeof(Vertex);

    auto commandList = Window::get_instance()->get_cmd_list();
    ID3D12Resource* intermediateVertexBuffer;
    Renderer::update_buffer_resource(commandList,
        &m_vertex_buffer, &intermediateVertexBuffer,
        no_of_vertices, sizeof(Vertex), data);
    intermediateVertexBuffer->Release();

    m_VertexBufferView.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
    m_VertexBufferView.SizeInBytes = sizeof(Vertex) * vertices_count;
    m_VertexBufferView.StrideInBytes = sizeof(Vertex);
}

ID3D12Resource* VertexBuffer::get() const
{
    return m_vertex_buffer;
}
