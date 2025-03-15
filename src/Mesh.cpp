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
#include "GreedyMeshletizer/nvMeshletizer.h"
#include "Tools/GPUProfiler.h"
#include "Tools/MeshletBenchmark.h"

#define TRACY_NO_SAMPLE_BRANCH
#define TRACY_NO_SAMPLE_RETIREMENT

#include "tracy/Tracy.hpp"



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
    else if (m_type == GREEDY)
        meshletizeGreedy();
    else if (m_type == BSPHERE)
        meshletizeBoundingSphere();
    else if (m_type == NVIDIA)
        meshletizeNvidia();



    generateSubsets();

    for (int i = 0; i < m_subsets.size(); i++)
    {
        m_meshInfoBuffers.push_back(new ConstantBuffer<MeshInfo>("MeshInfo"));
    }


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
    m_MeshletMaxPrims = maxPrims;
    m_MeshletMaxVerts = maxVerts;

    generateSubsets();
    for (int i = 0; i < m_subsets.size(); i++)
    {
        m_meshInfoBuffers.push_back(new ConstantBuffer<MeshInfo>("MeshInfo"));
    }

    float totalRadiuses = 0.0f;
    float totalAngles = 0.0f;
    float avgRadius = 0.0f;
    float maxRadius = 0.0f;
    float minRadius = 0.0f;
    float avgAngle = 0.0f;
    int degenerateConeCounter = 0;
    for (int i = 0; i < m_cullData.size(); i++)
    {
        totalRadiuses += m_cullData[i].BoundingSphere.w;
        if(m_cullData[i].NormalCone[3] == 0xff)
        {
            degenerateConeCounter++;
        }
        float angle = float((m_cullData[i].NormalCone[3] >> 24) & 0xFF);
        totalAngles += acosf(angle);
        if (m_cullData[i].BoundingSphere.w > maxRadius)
        {
            maxRadius = m_cullData[i].BoundingSphere.w;
        }
        if (m_cullData[i].BoundingSphere.w < minRadius)
        {
            minRadius = m_cullData[i].BoundingSphere.w;
        }



    }

    avgRadius = totalRadiuses / m_cullData.size();
    avgAngle = totalAngles / m_cullData.size();
    //float vertFill = (float)totalVerts / (float)(m_MeshletMaxVerts * m_meshlets.size());
    //float triFill = (float)totalTris / (float)(m_MeshletMaxPrims * m_meshlets.size());
    printf("=========MESHLETIZER %i =========\n", static_cast<int>(m_type));
    printf("Avg radius: %f \n", avgRadius);
    printf("Avg angle: %f \n", avgAngle);
    printf("Max radius: %f \n", maxRadius);
    printf("Min radius: %f \n", minRadius);
    printf("Meshlets: %i \n", m_meshlets.size());
    printf("Degenerate cones: %i \n", degenerateConeCounter);

}

Mesh::~Mesh()
{
    for (int i = 0; i < m_meshInfoBuffers.size(); i++)
    {
        delete m_meshInfoBuffers[i];
    }
    m_meshInfoBuffers.clear();
    delete VertexResource;
    delete IndexResource;
    delete MeshletResource;
    delete MeshletTriangleIndicesResource;
    delete CullDataResource;
    for (auto& texture : m_textures)
    {
        delete texture;
    }
}


void Mesh::bindTextures(PipelineState* pso)
{
    if (m_textures.size() == 0) 
        return;
    std::vector<ID3D12DescriptorHeap*> heaps = std::vector<ID3D12DescriptorHeap*>(m_textures.size());
    for (int i = 0; i < m_textures.size(); i++)
    {
        heaps[i] = m_textures[i]->heap;
    }

    auto command_list = Renderer::get_instance()->g_pd3dCommandList;

    command_list->SetDescriptorHeaps(m_textures.size(), heaps.data());

    for (int i = 0; i < m_textures.size(); i++)
    {
        if (m_textures[i]->type == TextureType::Diffuse)
        {
            auto index = pso->getRootParameterIndex("DIFFUSE_TEX");
            command_list->SetGraphicsRootDescriptorTable(index, m_textures[i]->heap->GetGPUDescriptorHandleForHeapStart());
        }
    }
}

void Mesh::bindMeshInfo(uint32_t meshletCount, uint32_t meshletOffset, uint32_t subsetIndex, PipelineState* pso)
{
    MeshInfo info;
    info.IndexBytes = sizeof(uint32_t);
    info.MeshletCount = meshletCount;
    info.MeshletOffset = meshletOffset;
    m_meshInfoBuffers[subsetIndex]->uploadData(info);
    m_meshInfoBuffers[subsetIndex]->setConstantBuffer(pso);
}

void Mesh::dispatch(PipelineState* pso)
{
    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    bindTextures(pso);
    VertexResource->bindResource(pso, "Vertices");
    MeshletResource->bindResource(pso, "Meshlets");
    IndexResource->bindResource(pso, "IndexBuffer");
    MeshletTriangleIndicesResource->bindResource(pso, "LocalIndexBuffer");
    CullDataResource->bindResource(pso, "meshletcullData");

    auto profilerEntry = GPUProfiler::getInstance()->startEntry(cmd_list, "Dispatch Mesh");
    {
#ifdef CULLING
        // We don't really need subsets in case we have culling, as throught AS we can dispatch at most 32 * 65535 meshlets, so about 2 milions
        // if we ever have more than 2 milions meshlets I suppose we can start praying
        bindMeshInfo(m_meshlets.size(), 0, 0);
        cmd_list->DispatchMesh(hlsl::divRoundUp(m_meshlets.size(), 32), 1, 1);

#else
        int i = 0;
        for (auto& subset : m_subsets)
        {
            
            bindMeshInfo(subset.size, subset.offset, i, pso);

            cmd_list->DispatchMesh(subset.size, 1, 1);

            i++;
        }
#endif
    } GPUProfiler::getInstance()->endEntry(cmd_list, profilerEntry);
    
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
    {
        ZoneScopedN("DXMESH meshletizing");
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
    }



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
    std::vector<unsigned char> meshlet_triangles(max_meshlets * m_MeshletMaxPrims);

    size_t meshlet_count;
    auto benchmark = MeshletBenchmark::getInstance();
    {
        ZoneScopedN("Meshoptimizer meshletizing");
        benchmark->startMeshletizing();
            meshlet_count = meshopt_buildMeshlets(
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

        for(int i = 0; i < meshlets.size(); i++)
        {
            meshopt_optimizeMeshlet(indices_mapping.data() + meshlets[i].vertex_offset, meshlet_triangles.data() + meshlets[i].triangle_offset, meshlets[i].triangle_count, meshlets[i].vertex_count);
        }
        benchmark->endMeshletizing();
    }
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
    {
        ZoneScopedN("Greedy meshletizing");
        benchmark->startMeshletizing();
        meshletizers::greedy::meshletize(m_MeshletMaxVerts, m_MeshletMaxPrims, m_indices, m_vertices, m_meshlets, uniqueVertexIndices, m_meshletTriangles);
        benchmark->endMeshletizing();
    }
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

void Mesh::meshletizeBoundingSphere()
{
    std::vector<uint32_t> newIndices(m_indices.size());
    meshopt_optimizeVertexCache(newIndices.data(), m_indices.data(), m_indices.size(), m_vertices.size());
    m_indices = newIndices;
    std::vector<uint32_t> uniqueVertexIndices;
    std::vector<uint32_t> indicesMapping;
    auto benchmark = MeshletBenchmark::getInstance();
    benchmark->startMeshletizing();
    {
        ZoneScopedN("BS meshletizing");
        meshletizers::boundingSphere::meshletize(m_MeshletMaxVerts, m_MeshletMaxPrims, m_indices, m_vertices, m_meshlets, uniqueVertexIndices, m_meshletTriangles);
        benchmark->endMeshletizing();
    }
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

void Mesh::meshletizeNvidia()
{
    std::vector<uint32_t> uniqueVertexIndices;
    std::vector<uint32_t> indicesMapping;
    auto benchmark = MeshletBenchmark::getInstance();
    benchmark->startMeshletizing();
    {
        ZoneScopedN("NV meshletizing");
        meshletizers::nvidia::meshletize(m_MeshletMaxVerts, m_MeshletMaxPrims, m_indices, m_vertices, m_meshlets, uniqueVertexIndices, m_meshletTriangles);
    }
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
    else if (m_type == GREEDY)
        meshletizeGreedy();
    else if (m_type == BSPHERE)
        meshletizeBoundingSphere();
    else if (m_type == NVIDIA)
        meshletizeNvidia();

    generateSubsets();
}

void Mesh::generateSubsets()
{
    int meshletsNumber = m_meshlets.size();
    int subsetsNumber = hlsl::divRoundUp(meshletsNumber, 65535);
    for (int i = 0; i < subsetsNumber; i++)
    {
        MeshSubset subset;
        subset.offset = i * 65535;
        subset.size = ((subset.offset + 65535) > meshletsNumber) ? meshletsNumber - subset.offset : 65535;
        m_subsets.push_back(subset);
    }
}
