#include "Mesh.h"
#include "Renderer.h"
#include <DirectXMesh.h>
#include <utils/ErrorHandler.h>
#include <utility>

#include "meshoptimizer.h"
#include "utils/Utils.h"

Mesh::Mesh(std::vector<Vertex> const& vertices, std::vector<u32> const& indices, std::vector<Texture*> const& textures, std::vector<hlsl::float3> const& positions, std::vector<hlsl::float3> const& normals, std::vector<hlsl::float2> const& UVS, std::vector<u32> const& attributes, MeshletizerType meshletizerType)
{
    m_vertices = vertices;
    m_indices = indices;
    m_textures = textures;
    m_UVs = UVS;
    m_positions = positions;
    m_normals = normals;
    m_UVs = UVS;
    m_attributes = attributes;
    m_type = meshletizerType;
    if(m_type == MESHOPT)
        meshletize_meshoptimizer();
    else
        meshletize_dxmesh();
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
    if (m_type == MESHOPT)
    {
        cmd_list->SetGraphicsRootShaderResourceView(4, IndexResource->GetGPUVirtualAddress());
        cmd_list->SetGraphicsRootShaderResourceView(5, MeshletTriangleIndicesResource->GetGPUVirtualAddress());
    }
    else
    {
        cmd_list->SetGraphicsRootShaderResourceView(4, UniqueVertexIndexResource->GetGPUVirtualAddress());
        cmd_list->SetGraphicsRootShaderResourceView(5, PrimitiveIndexResource->GetGPUVirtualAddress());
    }

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
        // Pull out some final counts for readability
    const uint32_t vertexCount = static_cast<uint32_t>(m_vertices.size());
    const uint32_t triCount = m_indices.size() / 3;

    std::vector<hlsl::float3> positionReorder;
    std::vector<hlsl::float3> normalReorder;
    std::vector<hlsl::float2> uvReorder;
    std::vector<uint32_t> indexReorder;
    std::vector<uint32_t> faceRemap;
    std::vector<uint32_t> vertexRemap;
    std::vector<uint32_t> dupVerts;
    std::vector<Subset> indexSubsets;
    std::vector<Subset> m_meshletSubsets;

    std::vector<hlsl::float3> tangents;
    std::vector<hlsl::float3> bitangents;

    // Resize all our interim data buffers to appropriate sizes for the mesh
    positionReorder.resize(vertexCount);
    indexReorder.resize(m_indices.size());

    faceRemap.resize(triCount);
    vertexRemap.resize(vertexCount);

    ///
    // Use DirectXMesh to optimize our vertex buffer data

    // Clean the mesh, sort faces by material, and reorder


    AssertFailed(DirectX::Clean(m_indices.data(), triCount, vertexCount, nullptr, m_attributes.data(), dupVerts, true));
    AssertFailed(DirectX::AttributeSort(triCount, m_attributes.data(), faceRemap.data()));
    AssertFailed(DirectX::ReorderIB(m_indices.data(), triCount, faceRemap.data(), indexReorder.data()));

    std::swap(m_indices, indexReorder);

    // Optimize triangle faces and reorder
    AssertFailed(DirectX::OptimizeFacesLRU((m_indices.data()), triCount, faceRemap.data()));
    AssertFailed(DirectX::ReorderIB((m_indices.data()), triCount, faceRemap.data(), indexReorder.data()));

    std::swap(m_indices, indexReorder);

    // Optimize our vertex data
    // This line crashes when switching to DXMESH (but not when starting up with it)
    AssertFailed(DirectX::OptimizeVertices(m_indices.data(), triCount, vertexCount, vertexRemap.data()));

    // Finalize the index & vertex buffers (potential reordering)
    AssertFailed(DirectX::FinalizeIB(m_indices.data(), triCount, vertexRemap.data(), vertexCount, indexReorder.data()));
    AssertFailed(DirectX::FinalizeVB(m_positions.data(), sizeof(hlsl::float3), vertexCount, dupVerts.data(), dupVerts.size(), vertexRemap.data(), positionReorder.data()));

    std::swap(m_indices, indexReorder);
    std::swap(m_positions, positionReorder);

    //if (HasAttribute(m_type, Attribute::Normal))
    {
        normalReorder.resize(vertexCount);
        AssertFailed(DirectX::FinalizeVB(m_normals.data(), sizeof(hlsl::float3), vertexCount, dupVerts.data(), dupVerts.size(), vertexRemap.data(), normalReorder.data()));

        std::swap(m_normals, normalReorder);
    }

    //if (HasAttribute(m_type, Attribute::TexCoord))
    {
        uvReorder.resize(vertexCount);
        AssertFailed(DirectX::FinalizeVB(m_UVs.data(), sizeof(hlsl::float2), vertexCount, dupVerts.data(), dupVerts.size(), vertexRemap.data(), uvReorder.data()));

        std::swap(m_UVs, uvReorder);
    }

    // Populate material subset data
    auto subsets = DirectX::ComputeSubsets(m_attributes.data(), m_attributes.size());

    indexSubsets.resize(subsets.size());
    for (uint32_t i = 0; i < subsets.size(); ++i)
    {
        indexSubsets[i].Offset = static_cast<uint32_t>(subsets[i].first) * 3;
        indexSubsets[i].Count = static_cast<uint32_t>(subsets[i].second) * 3;
    }

    {
        tangents.resize(vertexCount);
        bitangents.resize(vertexCount);

        AssertFailed(ComputeTangentFrame(
            m_indices.data(),
            triCount,
            reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
            reinterpret_cast<const DirectX::XMFLOAT3*>(m_normals.data()),
            reinterpret_cast<const DirectX::XMFLOAT2*>(m_UVs.data()),
            vertexCount,
            reinterpret_cast<DirectX::XMFLOAT3*>(tangents.data()),
            reinterpret_cast<DirectX::XMFLOAT3*>(bitangents.data())));
    }

    // Meshletize our mesh and generate per-meshlet culling data
    AssertFailed(ComputeMeshlets(
        m_MeshletMaxVerts,
        m_MeshletMaxPrims,
        m_indices.data(),
        m_indices.size(),
        indexSubsets.data(),
        static_cast<uint32_t>(indexSubsets.size()),
        reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
        static_cast<uint32_t>(m_positions.size()),
        m_meshletSubsets,
        m_meshlets,
        m_uniqueVertexIndices,
        m_primitiveIndices
    ));

    //m_cullData.resize(m_meshlets.size());

    //AssertFailed(ComputeCullData(
    //    reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
    //    m_positions.size(),
    //    m_meshlets.data(),
    //    m_meshlets.size(),
    //    reinterpret_cast<uint16_t*>(m_uniqueVertexIndices.data()),
    //    m_primitiveIndices.data(),
    //    DirectX::CNORM_DEFAULT,
    //    m_cullData.data()
    //));
}

void Mesh::meshletize_meshoptimizer()
{
    const float cone_weight = 0.0f;

    size_t max_meshlets = meshopt_buildMeshletsBound(m_indices.size(), m_MeshletMaxVerts, m_MeshletMaxPrims);
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    // vertex index data, so every entry in that vector is a global index of a vertex

    m_indices_mapping.resize(max_meshlets * m_MeshletMaxVerts);
    std::vector<unsigned char> meshlet_triangles(max_meshlets * m_MeshletMaxPrims * 3);

    size_t meshlet_count = meshopt_buildMeshlets(
        meshlets.data(),
        m_indices_mapping.data(),
        meshlet_triangles.data(),
        m_indices.data(),
        m_indices.size(),
        &m_positions[0].x,
        m_positions.size(),
        sizeof(hlsl::float3),
        m_MeshletMaxVerts,
        m_MeshletMaxPrims,
        cone_weight);

    m_meshlets.clear();
    m_meshlets.resize(meshlet_count);
    uint32_t offset = 0;
    for (int i = 0; i < meshlet_count; i++)
    {
        m_meshlets[i].VertCount = meshlets[i].vertex_count;
        m_meshlets[i].PrimCount = meshlets[i].triangle_count;
        m_meshlets[i].VertOffset = meshlets[i].vertex_offset;
        m_meshlets[i].PrimOffset = meshlets[i].triangle_offset;
    }

    std::vector<uint32_t> final_meshlet_triangles(max_meshlets * m_MeshletMaxPrims);

    // We could move the packing to a seperate function
    // However, it could result 
    size_t triangle_count = meshlet_triangles.size() / 3;
    for (size_t i = 0; i < triangle_count; ++i)
    {
        final_meshlet_triangles[i] = olej_utils::pack_triangle(meshlet_triangles[i * 3 + 0], meshlet_triangles[i * 3 + 1], meshlet_triangles[i * 3 + 2]);
    }

    m_meshletTriangles = final_meshlet_triangles;
}

void Mesh::changeMeshletizerType(MeshletizerType type)
{
    if (type == m_type)
        return;

    m_type = type;

    m_meshletTriangles.clear();
    m_primitiveIndices.clear();
    m_uniqueVertexIndices.clear();
    m_meshlets.clear();

    if (type == MESHOPT)
        meshletize_meshoptimizer();
    else
        meshletize_dxmesh();
}
