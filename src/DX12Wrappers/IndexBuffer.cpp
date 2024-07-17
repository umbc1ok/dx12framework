#include "IndexBuffer.h"

#include "Renderer.h"
#include "Window.h"

IndexBuffer::IndexBuffer(u32 const* data, u32 const no_of_indicies)
{
    auto commandList = Window::get_instance()->get_cmd_list();

    ID3D12Resource* intermediateIndexBuffer;
    Renderer::update_buffer_resource(commandList,
        &m_IndexBuffer, &intermediateIndexBuffer,
        no_of_indicies, sizeof(u32), data);

    // Create index buffer view.
    m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_IndexBufferView.SizeInBytes = sizeof(u32) * no_of_indicies;
}

IndexBuffer::~IndexBuffer()
{
    m_IndexBuffer->Release();
}

ID3D12Resource* IndexBuffer::get() const
{
    return m_IndexBuffer;
}
