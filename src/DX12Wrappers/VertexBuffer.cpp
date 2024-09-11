#include "VertexBuffer.h"

#include <d3d12.h>

#include "Renderer.h"
#include "Vertex.h"
#include "Window.h"


VertexBuffer::VertexBuffer(Vertex const* data, u32 vertices_count)
{
    m_data = data;
    m_verticies_count = vertices_count;
    auto cmdqueue = Renderer::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto cmdlist = cmdqueue->get_command_list();
    ID3D12Resource* intermediateVertexBuffer;
    Renderer::get_instance()->update_buffer_resource(cmdlist,
        &m_vertex_buffer, &intermediateVertexBuffer,
        vertices_count, sizeof(Vertex), data);
    //intermediateVertexBuffer->Release();
    m_VertexBufferView = new D3D12_VERTEX_BUFFER_VIEW();
    m_VertexBufferView->BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
    m_VertexBufferView->SizeInBytes = sizeof(Vertex) * vertices_count;
    m_VertexBufferView->StrideInBytes = sizeof(Vertex);
    auto fence_value = cmdqueue->execute_command_list(cmdlist);
    cmdqueue->wait_for_fence_value(fence_value);
}

VertexBuffer::~VertexBuffer()
{
    m_vertex_buffer->Release();
}

ID3D12Resource* VertexBuffer::get() const
{
    return m_vertex_buffer;
}

void VertexBuffer::update()
{
    /*
    auto commandList = Window::get_instance()->get_cmd_list();
    ID3D12Resource* intermediateVertexBuffer;
    Renderer::update_buffer_resource(commandList,
        &m_vertex_buffer, &intermediateVertexBuffer,
        m_verticies_count, sizeof(Vertex), m_data);
    //intermediateVertexBuffer->Release();
    m_VertexBufferView = new D3D12_VERTEX_BUFFER_VIEW();
    m_VertexBufferView->BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
    m_VertexBufferView->SizeInBytes = sizeof(Vertex) * m_verticies_count;
    m_VertexBufferView->StrideInBytes = sizeof(Vertex);
    */
}
