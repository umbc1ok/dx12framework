#include "IndexBuffer.h"

#include "Renderer.h"
#include "Window.h"

IndexBuffer::IndexBuffer(u16 const* data, u16 const no_of_indicies)
{
    m_data = data;
    m_no_of_indicies = no_of_indicies;

    auto cmdqueue = Renderer::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto cmdlist = cmdqueue->get_command_list();
    ID3D12Resource* intermediateIndexBuffer;
    Renderer::get_instance()->update_buffer_resource(cmdlist,
        &m_IndexBuffer, &intermediateIndexBuffer,
        no_of_indicies, sizeof(u16), data);

    // Create index buffer view.
    m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_IndexBufferView.SizeInBytes = sizeof(u16) * no_of_indicies;
    auto fence_value = cmdqueue->execute_command_list(cmdlist);
    cmdqueue->wait_for_fence_value(fence_value);
}

IndexBuffer::~IndexBuffer()
{
    m_IndexBuffer->Release();
}

ID3D12Resource* IndexBuffer::get() const
{
    return m_IndexBuffer;
}
void IndexBuffer::update()
{
    /*
    auto commandList = Window::get_instance()->get_cmd_list();
    ID3D12Resource* intermediateIndexBuffer;
    Renderer::update_buffer_resource(commandList,
        &m_IndexBuffer, &intermediateIndexBuffer,
        m_no_of_indicies, sizeof(u16), m_data);

    // Create index buffer view.
    m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_IndexBufferView.SizeInBytes = sizeof(u16) * m_no_of_indicies;
    */
}