#include "Mesh.h"
#include "Renderer.h"

Mesh::Mesh(std::vector<Vertex> const& m_vertices , std::vector<u16> const& m_indices, std::vector<Texture*> const& m_textures)
{
    m_vertex_buffer = new VertexBuffer(m_vertices.data(), m_vertices.size());
    m_index_buffer = new IndexBuffer(m_indices.data(), m_indices.size());
    this->m_vertices = m_vertices;
    this->m_indices = m_indices;
    this->m_textures = m_textures;
}

void Mesh::draw()
{
    bind_textures();
    auto command_list = Renderer::get_instance()->g_pd3dCommandList;

    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->IASetVertexBuffers(0, 1, m_vertex_buffer->get_view());
    command_list->IASetIndexBuffer(m_index_buffer->get_view());
    command_list->DrawIndexedInstanced(m_indices.size(), 1, 0, 0, 0);

}

void Mesh::bind_textures()
{
    std::vector<ID3D12DescriptorHeap*> heaps = std::vector<ID3D12DescriptorHeap*>(m_textures.size());
    for (int i = 0; i < m_textures.size(); i++)
    {
        heaps[i] = m_textures[i]->heap;
    }

    auto command_list = Renderer::get_instance()->g_pd3dCommandList;
    command_list->SetDescriptorHeaps(m_textures.size(), heaps.data());

    // TODO: We are using just the first texture for now
    // This will proooobably cause nasty crashes from time to time, we will see
    if (m_textures.size() > 0)
    {
        command_list->SetGraphicsRootDescriptorTable(1, m_textures[0]->SRV_GPU);
    }
}
