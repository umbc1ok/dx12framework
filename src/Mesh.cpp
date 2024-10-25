#include "Mesh.h"
#include "Renderer.h"
#include <DirectXMesh.h>
#include <utils/ErrorHandler.h>
#include <utility>

Mesh::Mesh(std::vector<Vertex> const& vertices , std::vector<u16> const& indices, std::vector<Texture*> const& textures, std::vector<hlsl::float3> const& positions, std::vector<hlsl::float3> const& normals, std::vector<hlsl::float2> const& UVS, std::vector<u32> attributes)
{
    m_vertices = vertices;
    m_indices = indices;
    m_textures = textures;
    m_UVs = UVS;
    m_vertex_buffer = new VertexBuffer(m_vertices.data(), m_vertices.size());
    m_index_buffer = new IndexBuffer(m_indices.data(), m_indices.size());
    m_positions = positions;
    m_normals = normals;
    m_UVs = UVS;
    m_attributes = attributes;
    meshletize();
}

void Mesh::draw()
{
    /*
    bind_textures();
    auto command_list = Renderer::get_instance()->g_pd3dCommandList;

    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->IASetVertexBuffers(0, 1, m_vertex_buffer->get_view());
    command_list->IASetIndexBuffer(m_index_buffer->get_view());
    command_list->DrawIndexedInstanced(m_indices.size(), 1, 0, 0, 0);
    */
    dispatch();
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

void Mesh::dispatch()
{
    
    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    cmd_list->SetGraphicsRoot32BitConstant(1, mesh_info.IndexSize, 0);
    cmd_list->SetGraphicsRootShaderResourceView(2, VertexResources[0]->GetGPUVirtualAddress());
    cmd_list->SetGraphicsRootShaderResourceView(3, MeshletResource->GetGPUVirtualAddress());
    cmd_list->SetGraphicsRootShaderResourceView(4, UniqueVertexIndexResource->GetGPUVirtualAddress());
    cmd_list->SetGraphicsRootShaderResourceView(5, PrimitiveIndexResource->GetGPUVirtualAddress());

    for (auto& subset : m_meshletSubsets)
    {
        cmd_list->SetGraphicsRoot32BitConstant(1, subset.Offset, 1);
        cmd_list->DispatchMesh(subset.Count, 1, 1);
    }
    
}

void Mesh::meshletize()
{
    // Pull out some final counts for readability
    const uint32_t vertexCount = static_cast<uint32_t>(m_vertices.size());
    const uint32_t triCount = m_indices.size() / 3;

    // Resize all our interim data buffers to appropriate sizes for the mesh
    m_positionReorder.resize(vertexCount);
    m_indexReorder.resize(m_indices.size());

    m_faceRemap.resize(triCount);
    m_vertexRemap.resize(vertexCount);

    ///
    // Use DirectXMesh to optimize our vertex buffer data

    // Clean the mesh, sort faces by material, and reorder
    // atributes are wrong

    
    AssertFailed(DirectX::Clean(m_indices.data(), triCount, vertexCount, nullptr, m_attributes.data(), m_dupVerts, true));
    AssertFailed(DirectX::AttributeSort(triCount, m_attributes.data(), m_faceRemap.data()));
    AssertFailed(DirectX::ReorderIB(m_indices.data(), triCount, m_faceRemap.data(), m_indexReorder.data()));

    std::swap(m_indices, m_indexReorder);

    // Optimize triangle faces and reorder
    AssertFailed(DirectX::OptimizeFacesLRU((m_indices.data()), triCount, m_faceRemap.data()));
    AssertFailed(DirectX::ReorderIB((m_indices.data()), triCount, m_faceRemap.data(), m_indexReorder.data()));

    std::swap(m_indices, m_indexReorder);

    // Optimize our vertex data
    AssertFailed(DirectX::OptimizeVertices(m_indices.data(), triCount, vertexCount, m_vertexRemap.data()));

    // Finalize the index & vertex buffers (potential reordering)
    AssertFailed(DirectX::FinalizeIB(m_indices.data(), triCount, m_vertexRemap.data(), vertexCount, m_indexReorder.data()));
    AssertFailed(DirectX::FinalizeVB(m_positions.data(), sizeof(hlsl::float3), vertexCount, m_dupVerts.data(), m_dupVerts.size(), m_vertexRemap.data(), m_positionReorder.data()));

    std::swap(m_indices, m_indexReorder);
    std::swap(m_positions, m_positionReorder);

    //if (HasAttribute(m_type, Attribute::Normal))
    {
        m_normalReorder.resize(vertexCount);
        AssertFailed(DirectX::FinalizeVB(m_normals.data(), sizeof(hlsl::float3), vertexCount, m_dupVerts.data(), m_dupVerts.size(), m_vertexRemap.data(), m_normalReorder.data()));

        std::swap(m_normals, m_normalReorder);
    }

    //if (HasAttribute(m_type, Attribute::TexCoord))
    {
        m_uvReorder.resize(vertexCount);
        AssertFailed(DirectX::FinalizeVB(m_UVs.data(), sizeof(hlsl::float2), vertexCount, m_dupVerts.data(), m_dupVerts.size(), m_vertexRemap.data(), m_uvReorder.data()));

        std::swap(m_UVs, m_uvReorder);
    }
    
    // Populate material subset data
    auto subsets = DirectX::ComputeSubsets(m_attributes.data(), m_attributes.size());

    m_indexSubsets.resize(subsets.size());
    for (uint32_t i = 0; i < subsets.size(); ++i)
    {
        m_indexSubsets[i].Offset = static_cast<uint32_t>(subsets[i].first) * 3;
        m_indexSubsets[i].Count = static_cast<uint32_t>(subsets[i].second) * 3;
    }

    {
        m_tangents.resize(vertexCount);
        m_bitangents.resize(vertexCount);

        AssertFailed(ComputeTangentFrame(
            m_indices.data(),
            triCount,
            reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
            reinterpret_cast<const DirectX::XMFLOAT3*>(m_normals.data()),
            reinterpret_cast<const DirectX::XMFLOAT2*>(m_UVs.data()),
            vertexCount,
            reinterpret_cast<DirectX::XMFLOAT3*>(m_tangents.data()),
            reinterpret_cast<DirectX::XMFLOAT3*>(m_bitangents.data())));
    }

    // Meshletize our mesh and generate per-meshlet culling data
    AssertFailed(ComputeMeshlets(
        MeshletMaxVerts,
        MeshletMaxPrims,
        m_indices.data(),
        m_indices.size(),
        m_indexSubsets.data(),
        static_cast<uint32_t>(m_indexSubsets.size()),
        reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
        static_cast<uint32_t>(m_positions.size()),
        m_meshletSubsets,
        m_meshlets,
        m_uniqueVertexIndices,
        m_primitiveIndices
    ));

    m_cullData.resize(m_meshlets.size());

    AssertFailed(ComputeCullData(
        reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
        m_positions.size(),
        m_meshlets.data(), 
        m_meshlets.size(),
        reinterpret_cast<uint16_t*>(m_uniqueVertexIndices.data()),
        m_primitiveIndices.data(),
        DirectX::CNORM_DEFAULT,
        m_cullData.data()
    ));
}
