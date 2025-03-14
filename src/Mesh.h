#pragma once
#include <vector>


#include "DX12Wrappers/Vertex.h"
#include "Texture.h"

#include "MeshletStructs.h"
#include "DX12Wrappers/Resource.h"
#include "../res/shaders/shared/shared_cb.h"


class PipelineState;

struct Attribute
{
    enum EType : uint32_t
    {
        Position,
        Normal,
        TexCoord,
        Tangent,
        Bitangent,
        Count
    };

    EType    Type;
    uint32_t Offset;
};

struct MeshSubset
{
    uint32_t offset;
    uint32_t size;
};

template <typename T>
class ConstantBuffer;


class Mesh
{
public:
    Mesh(std::vector<Vertex> const& vertices,
        std::vector<uint32_t> const&  indices,
        std::vector<Texture*> const& textures,
        std::vector<hlsl::float3> const& positions,
        std::vector<hlsl::float3> const& normals,
        std::vector<hlsl::float2> const& UVS,
        std::vector<uint32_t> const& attributes,
        MeshletizerType meshletizerType,
        int32_t maxVerts,
        int32_t maxPrims);


    Mesh(std::vector<Vertex> const& vertices, 
        std::vector<uint32_t> const&  indices,
        std::vector<Texture*> const& textures,
        std::vector<hlsl::float3> const& positions,
        std::vector<hlsl::float3> const& normals,
        std::vector<hlsl::float2> const& UVS,
        std::vector<uint32_t> const& attributes,
        MeshletizerType meshletizerType,
        int32_t maxVerts,
        int32_t maxPrims,
        std::vector<Meshlet> const& meshlets,
        std::vector<uint32_t> const& meshletTriangles,
        std::vector<CullData> const&  cullData);

    ~Mesh();

    void bindTextures();
    void bindMeshInfo(uint32_t meshletCount, uint32_t meshletOffset, uint32_t subsetIndex, PipelineState* pso);

    void dispatch(PipelineState* pso);

    void meshletizeDXMESH();
    void meshletizeMeshoptimizer();
    void meshletizeGreedy();
    void meshletizeBoundingSphere();
    void meshletizeNvidia();



    void changeMeshletizerType(MeshletizerType type);


    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<CullData> m_cullData;

    std::vector<Meshlet> m_meshlets;
    std::vector<uint32_t> m_meshletTriangles;

    std::vector<Texture*> m_textures;
    std::vector<uint32_t> m_attributes;

    std::vector<hlsl::float3> m_positions;
    std::vector<hlsl::float3> m_normals;
    std::vector<hlsl::float2> m_UVs;

    std::vector<MeshSubset> m_subsets;

    Resource*              VertexResource;
    Resource*              IndexResource;
    Resource*              MeshletResource;
    Resource*              MeshletTriangleIndicesResource;
    Resource*              CullDataResource;

    std::vector<ConstantBuffer<MeshInfo>*> m_meshInfoBuffers;

    int32_t m_MeshletMaxVerts = 64;
    int32_t m_MeshletMaxPrims = 124;

    MeshletizerType m_type = MESHOPT;
private:
    void generateSubsets();
};

