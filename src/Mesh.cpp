#include "Mesh.h"
#include "Renderer.h"
#include <DirectXMesh.h>
#include <utils/ErrorHandler.h>
#include <utility>

#include "meshoptimizer.h"
#include "GreedyMeshletizer/GreedyMeshletizer.h"
#include "utils/Utils.h"
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"
#include "DX12Wrappers/ConstantBuffer.h"
#include "GreedyMeshletizer/boundingSphereMeshletizer.h"
#include "Tools/MeshletBenchmark.h"


Mesh::Mesh(std::vector<Vertex> const& vertices, std::vector<uint32_t> const& indices, std::vector<Texture*> const& textures, std::vector<hlsl::float3> const& positions, std::vector<hlsl::float3> const& normals, std::vector<hlsl::float2> const& UVS, std::vector<uint32_t> const& attributes, MeshletizerType meshletizerType, int32_t maxVerts, int32_t maxPrims)
{
    m_vertices = vertices;
    m_indices = indices;
    m_textures = textures;
    m_UVs = UVS;
    m_positions = positions;
    m_normals = normals;
    m_attributes = attributes;
    m_type = meshletizerType;
    m_MeshletMaxPrims = maxPrims;
    m_MeshletMaxVerts = maxVerts;

    if(m_type == MESHOPT)
        meshletizeMeshoptimizer();
    else if (m_type == DXMESH)
        meshletizeDXMESH();
    else
        meshletizeGreedy();

    generateSubsets();

    m_meshInfoBuffer = new ConstantBuffer<MeshInfo>();


}

Mesh::Mesh(std::vector<Vertex> const& vertices, std::vector<uint32_t> const& indices, std::vector<Texture*> const& textures,
    std::vector<hlsl::float3> const& positions, std::vector<hlsl::float3> const& normals,
    std::vector<hlsl::float2> const& UVS, std::vector<uint32_t> const& attributes, MeshletizerType meshletizerType,
    int32_t maxVerts, int32_t maxPrims,
    std::vector<Meshlet> const& meshlets, std::vector<uint32_t> const& meshletTriangles, std::vector<CullData> const& cullData)
{
    m_vertices = vertices;
    m_indices = indices;
    m_textures = textures;
    m_UVs = UVS;
    m_positions = positions;
    m_normals = normals;
    m_attributes = attributes;
    m_meshlets = meshlets;
    m_type = meshletizerType;
    m_meshletTriangles = meshletTriangles;
    m_cullData = cullData;
    m_meshInfoBuffer = new ConstantBuffer<MeshInfo>();
    m_MeshletMaxPrims = maxPrims;
    m_MeshletMaxVerts = maxVerts;

    generateSubsets();

}

void Mesh::draw()
{
    dispatch();
}

void Mesh::bindTextures()
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

void Mesh::bindMeshInfo(uint32_t meshletCount, uint32_t meshletOffset)
{
    MeshInfo info;
    info.IndexBytes = sizeof(uint32_t);
    info.MeshletCount = meshletCount;
    info.MeshletOffset = meshletOffset;
    m_meshInfoBuffer->uploadData(info);
    m_meshInfoBuffer->setConstantBuffer(1);
}

void Mesh::dispatch()
{
    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    cmd_list->SetGraphicsRootShaderResourceView(3, VertexResource->getGPUVirtualAddress());
    cmd_list->SetGraphicsRootShaderResourceView(4, MeshletResource->getGPUVirtualAddress());
    cmd_list->SetGraphicsRootShaderResourceView(5, IndexResource->getGPUVirtualAddress());
    cmd_list->SetGraphicsRootShaderResourceView(6, MeshletTriangleIndicesResource->getGPUVirtualAddress());
    // TODO: Bind Cull Data below
    cmd_list->SetGraphicsRootShaderResourceView(7, CullDataResource->getGPUVirtualAddress());

    for (auto& subset : m_subsets)
    {
        bindMeshInfo(subset.size, subset.offset);
        cmd_list->DispatchMesh(subset.size, 1, 1);
    }
    
}

void Mesh::meshletizeDXMESH()
{
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
    std::vector<Subset> meshlet_subsets;
    std::vector<uint8_t> unique_vertex_indices;
    std::vector<PackedTriangle> primitive_indices;
    std::vector<uint32_t> indices_mapping;


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

    //// Optimize triangle faces and reorder
    AssertFailed(DirectX::OptimizeFacesLRU((m_indices.data()), triCount, faceRemap.data()));
    AssertFailed(DirectX::ReorderIB((m_indices.data()), triCount, faceRemap.data(), indexReorder.data()));

    std::swap(m_indices, indexReorder);

    // tu sie cos jebie
    // assimp should be doing that already, so comment out for now
    //AssertFailed(DirectX::OptimizeVertices(m_indices.data(), triCount, vertexCount, vertexRemap.data()));

    //// Finalize the index & vertex buffers (potential reordering)
    //AssertFailed(DirectX::FinalizeIB(m_indices.data(), triCount, vertexRemap.data(), vertexCount, indexReorder.data()));
    //AssertFailed(DirectX::FinalizeVB(m_vertices.data(), sizeof(Vertex), vertexCount, dupVerts.data(), dupVerts.size(), vertexRemap.data(), positionReorder.data()));

    //std::swap(m_indices, indexReorder);
    //std::swap(m_vertices, vertex);

    ////if (HasAttribute(m_type, Attribute::Normal))
    //{
    //    normalReorder.resize(vertexCount);
    //    AssertFailed(DirectX::FinalizeVB(m_normals.data(), sizeof(hlsl::float3), vertexCount, dupVerts.data(), dupVerts.size(), vertexRemap.data(), normalReorder.data()));

    //    std::swap(m_normals, normalReorder);
    //}

    ////if (HasAttribute(m_type, Attribute::TexCoord))
    //{
    //    uvReorder.resize(vertexCount);
    //    AssertFailed(DirectX::FinalizeVB(m_UVs.data(), sizeof(hlsl::float2), vertexCount, dupVerts.data(), dupVerts.size(), vertexRemap.data(), uvReorder.data()));

    //    std::swap(m_UVs, uvReorder);
    //}

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

    auto benchmark = MeshletBenchmark::getInstance();
    benchmark->startMeshletizing();


    AssertFailed(ComputeMeshlets(
        m_MeshletMaxVerts,
        m_MeshletMaxPrims,
        m_indices.data(),
        m_indices.size(),
        indexSubsets.data(),
        static_cast<uint32_t>(indexSubsets.size()),
        reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
        static_cast<uint32_t>(m_positions.size()),
        meshlet_subsets,
        m_meshlets,
        unique_vertex_indices,
        primitive_indices
    ));
    benchmark->endMeshletizing();



    m_cullData.resize(m_meshlets.size());
    AssertFailed(ComputeCullData(
        reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
        m_positions.size(),
        m_meshlets.data(),
        m_meshlets.size(),
        reinterpret_cast<uint32_t*>(unique_vertex_indices.data()),
        primitive_indices.data(),
        DirectX::CNORM_DEFAULT,
        m_cullData.data()
    ));

    m_meshletTriangles.resize(primitive_indices.size());

    for (int i = 0; i < primitive_indices.size(); i++)
    {
        m_meshletTriangles[i] = olej_utils::packTriangle(static_cast<uint8_t>(primitive_indices[i].indices.i0), static_cast<uint8_t>(primitive_indices[i].indices.i1), static_cast<uint8_t>(primitive_indices[i].indices.i2));
    }

    for(int i = 0; i < unique_vertex_indices.size(); i += 4)
    {
        uint32_t packed =
            static_cast<uint32_t>(unique_vertex_indices[i + 0]) << 0 |
            static_cast<uint32_t>(unique_vertex_indices[i + 1]) << 8 |
            static_cast<uint32_t>(unique_vertex_indices[i + 2]) << 16 |
            static_cast<uint32_t>(unique_vertex_indices[i + 3]) << 24;

        indices_mapping.push_back(packed);
    }
    m_indices = indices_mapping;
    //m_cullData.resize(m_meshlets.size());


}

void Mesh::meshletizeMeshoptimizer()
{
    const float cone_weight = 0.0f;

    size_t max_meshlets = meshopt_buildMeshletsBound(m_indices.size(), m_MeshletMaxVerts, m_MeshletMaxPrims);
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    std::vector<uint32_t> indices_mapping;
    // vertex index data, so every entry in that vector is a global index of a vertex

    indices_mapping.resize(max_meshlets * m_MeshletMaxVerts);
    std::vector<unsigned char> meshlet_triangles(max_meshlets * m_MeshletMaxPrims * 3);


    auto benchmark = MeshletBenchmark::getInstance();
    benchmark->startMeshletizing();
    size_t meshlet_count = meshopt_buildMeshlets(
        meshlets.data(),
        indices_mapping.data(),
        meshlet_triangles.data(),
        m_indices.data(),
        m_indices.size(),
        &m_positions[0].x,
        m_positions.size(),
        sizeof(hlsl::float3),
        m_MeshletMaxVerts,
        m_MeshletMaxPrims,
        cone_weight);
    benchmark->endMeshletizing();
    m_meshlets.clear();
    m_meshlets.resize(meshlet_count);
    int addedElements = 0;
    for (int i = 0; i < meshlet_count; i++)
    {
        m_meshlets[i].VertCount = meshlets[i].vertex_count;
        m_meshlets[i].PrimCount = meshlets[i].triangle_count;
        m_meshlets[i].VertOffset = meshlets[i].vertex_offset;
        m_meshlets[i].PrimOffset = meshlets[i].triangle_offset + addedElements;
        if(m_meshlets[i].PrimOffset % 3 == 1)
        {
            meshlet_triangles.insert(meshlet_triangles.begin() + m_meshlets[i].PrimOffset, meshlet_triangles.at(m_meshlets[i].PrimOffset));
            m_meshlets[i].PrimOffset++;
            meshlet_triangles.insert(meshlet_triangles.begin() + m_meshlets[i].PrimOffset, meshlet_triangles.at(m_meshlets[i].PrimOffset));
            m_meshlets[i].PrimOffset++;
            assert(m_meshlets[i].PrimOffset % 3 == 0);
            addedElements += 2;
        }
        else if (m_meshlets[i].PrimOffset % 3 == 2)
        {
            meshlet_triangles.insert(meshlet_triangles.begin() + m_meshlets[i].PrimOffset, meshlet_triangles.at(m_meshlets[i].PrimOffset));
            m_meshlets[i].PrimOffset++;
            assert(m_meshlets[i].PrimOffset % 3 == 0);
            addedElements++;
        }
        m_meshlets[i].PrimOffset /= 3;
    }

    std::vector<uint32_t> final_meshlet_triangles(meshlet_triangles.size() / 3);


    size_t triangle_count = meshlet_triangles.size() / 3;



    // convert data so it can be fed into ComputeCullData()
    std::vector<PackedTriangle> triangles(triangle_count);
    for(int i = 0; i < triangle_count; i++)
    {
        triangles[i].indices.i0 = meshlet_triangles[i * 3];
        triangles[i].indices.i1 = meshlet_triangles[i * 3 + 1];
        triangles[i].indices.i2 = meshlet_triangles[i * 3 + 2];
    }

    m_cullData.resize(m_meshlets.size());
    AssertFailed(ComputeCullData(
        reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
        m_positions.size(),
        m_meshlets.data(),
        m_meshlets.size(),
        indices_mapping.data(),
        triangles.data(),
        DirectX::CNORM_DEFAULT,
        m_cullData.data()
    ));




    for (size_t i = 0; i < triangle_count; ++i)
    {
        final_meshlet_triangles[i] = olej_utils::packTriangle(meshlet_triangles[i * 3 + 0], meshlet_triangles[i * 3 + 1], meshlet_triangles[i * 3 + 2]);
    }

    m_meshletTriangles = final_meshlet_triangles;
    m_indices.clear();
    m_indices = indices_mapping;
}

void Mesh::meshletizeGreedy()
{
    std::vector<uint32_t> newIndices(m_indices.size());
    meshopt_optimizeVertexCache(newIndices.data(), m_indices.data(), m_indices.size(), m_vertices.size());
    m_indices = newIndices;
    std::vector<uint32_t> uniqueVertexIndices;
    std::vector<uint32_t> indicesMapping;
    auto benchmark = MeshletBenchmark::getInstance();
    benchmark->startMeshletizing();
    meshletizers::boundingSphere::meshletize(m_MeshletMaxVerts, m_MeshletMaxPrims, m_indices, m_vertices, m_meshlets, uniqueVertexIndices, m_meshletTriangles);
    benchmark->endMeshletizing();
    m_indices = uniqueVertexIndices;


    // convert data so it can be fed into ComputeCullData()
    std::vector<PackedTriangle> triangles(m_meshletTriangles.size());
    for (int i = 0; i < triangles.size(); i++)
    {
        auto packed = m_meshletTriangles[i];
        triangles[i].indices.i0 = static_cast<uint8_t>(packed);
        triangles[i].indices.i1 = static_cast<uint8_t>(packed >> 8);
        triangles[i].indices.i2 = static_cast<uint8_t>(packed >> 16); 
    }


    m_cullData.resize(m_meshlets.size());

    AssertFailed(ComputeCullData(
        reinterpret_cast<const DirectX::XMFLOAT3*>(m_positions.data()),
        m_positions.size(),
        m_meshlets.data(),
        m_meshlets.size(),
        m_indices.data(),
        triangles.data(),
        DirectX::CNORM_DEFAULT,
        m_cullData.data()
    ));
}


void Mesh::changeMeshletizerType(MeshletizerType type)
{
    if (type == m_type)
        return;

    m_type = type;

    m_meshletTriangles.clear();
    m_meshlets.clear();

    if (type == MESHOPT)
        meshletizeMeshoptimizer();
    else if (type == DXMESH)
        meshletizeDXMESH();
    else
        meshletizeGreedy();

    generateSubsets();
}

void Mesh::generateSubsets()
{
    int meshletsNumber = m_meshlets.size();
    int subsetsNumber = (meshletsNumber / 65535) + 1;
    for (int i = 0; i < subsetsNumber; i++)
    {
        MeshSubset subset;
        subset.offset = i * 65535;
        subset.size = subset.offset + 65535 > meshletsNumber ? meshletsNumber - subset.offset : 65535;
        m_subsets.push_back(subset);
    }
}
