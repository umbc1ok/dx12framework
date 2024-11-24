#include "Mesh.h"
#include "Renderer.h"
#include <DirectXMesh.h>
#include <utils/ErrorHandler.h>
#include <utility>

#include "meshoptimizer.h"

Mesh::Mesh(std::vector<Vertex> const& vertices , std::vector<u32> const& indices, std::vector<Texture*> const& textures, std::vector<hlsl::float3> const& positions, std::vector<hlsl::float3> const& normals, std::vector<hlsl::float2> const& UVS, std::vector<u32> attributes)
{
    m_vertices = vertices;
    m_indices = indices;
    m_textures = textures;
    m_UVs = UVS;
    m_positions = positions;
    m_normals = normals;
    m_UVs = UVS;
    m_attributes = attributes;
    meshletize_meshoptimizer();
}

void Mesh::draw()
{
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
    cmd_list->SetGraphicsRootShaderResourceView(2, VertexResource->GetGPUVirtualAddress());
    cmd_list->SetGraphicsRootShaderResourceView(3, MeshletResource->GetGPUVirtualAddress());
    cmd_list->SetGraphicsRootShaderResourceView(4, IndexResource->GetGPUVirtualAddress());
    cmd_list->SetGraphicsRootShaderResourceView(5, MeshletTriangleIndicesResource->GetGPUVirtualAddress());

    // TODO: Add subsets
    //for (auto& subset : m_meshletSubsets)
    {
        cmd_list->SetGraphicsRoot32BitConstant(1, 0, 1);
        cmd_list->DispatchMesh(m_meshlets.size(), 1, 1);
    }
    
}

void Mesh::meshletize_dxmesh()
{
    // Add the DXMESH Implementation here
}

void Mesh::meshletize_meshoptimizer()
{

    const float cone_weight = 0.0f;

    size_t max_meshlets = meshopt_buildMeshletsBound(m_indices.size(), m_MeshletMaxVerts, m_MeshletMaxPrims);
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    // vertex index data, so every entry in that vector is a global index of a vertex
    std::vector<unsigned int> meshlet_vertices(max_meshlets * m_MeshletMaxVerts);
    std::vector<unsigned char> meshlet_triangles(max_meshlets * m_MeshletMaxPrims * 3);

    size_t meshlet_count = meshopt_buildMeshlets(
        meshlets.data(),
        meshlet_vertices.data(),
        meshlet_triangles.data(),
        m_indices.data(),
        m_indices.size(),
        &m_positions[0].x,
        m_positions.size(),
        sizeof(hlsl::float3),
        m_MeshletMaxVerts,
        m_MeshletMaxPrims,
        cone_weight);

    m_meshlets.resize(meshlet_count);
    for (int i = 0; i < meshlet_count; i++)
    {
        m_meshlets[i].VertCount = meshlets[i].vertex_count;
        m_meshlets[i].PrimCount = meshlets[i].triangle_count;
        m_meshlets[i].VertOffset = meshlets[i].vertex_offset;
        m_meshlets[i].PrimOffset = meshlets[i].triangle_offset;
    }

    m_indices = meshlet_vertices;
    std::vector<unsigned char> final_meshlet_triangles(max_meshlets * m_MeshletMaxPrims * 4);

    for (size_t i = 0; i < meshlet_triangles.size(); i += 3)
    {
        // Copy three bytes from meshlet_triangles
        final_meshlet_triangles[i * 4 / 3 + 0] = meshlet_triangles[i + 0];
        final_meshlet_triangles[i * 4 / 3 + 1] = meshlet_triangles[i + 1];
        final_meshlet_triangles[i * 4 / 3 + 2] = meshlet_triangles[i + 2];

        // Add the padding byte (fourth byte)
        final_meshlet_triangles[i * 4 / 3 + 3] = 0; // or another value if needed
    }

    m_meshletTriangles = final_meshlet_triangles;
}
