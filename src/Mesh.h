#pragma once
#include <vector>


#include "DX12Wrappers/IndexBuffer.h"
#include "DX12Wrappers/Vertex.h"
#include "DX12Wrappers/VertexBuffer.h"
#include "Texture.h"
#include <d3d12.h>
#include <wrl/client.h>

#include "DX12Wrappers/Resource.h"
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"

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
struct MeshInfo
{
    uint32_t IndexSize;
    uint32_t MeshletCount;

    uint32_t LastMeshletVertCount;
    uint32_t LastMeshletPrimCount;
};

enum MeshletizerType
{
    MESHOPT,
    DXMESH
};


class Mesh
{
public:
    Mesh(std::vector<Vertex> const& vertices, std::vector<u32> const& indices, std::vector<Texture*> const& textures, std::vector<hlsl::float3> const& positions, std::vector<hlsl::float3> const& normals, std::vector<hlsl::float2> const& UVS, std::vector<u32> attributes);
    ~Mesh() = default;

    void draw();
    void bind_textures();
    void dispatch();

    void meshletize_dxmesh();
    void meshletize_meshoptimizer();

    std::vector<Vertex> m_vertices;
    std::vector<u32> m_indices;
    std::vector<unsigned char> m_meshletTriangles;

    // Needed for DXMESH
    std::vector<PackedTriangle> m_primitiveIndices;
    std::vector<uint8_t> m_uniqueVertexIndices;


    std::vector<Texture*> m_textures;
    std::vector<uint32_t> m_attributes;

    std::vector<hlsl::float3> m_positions;
    std::vector<hlsl::float3> m_normals;
    std::vector<hlsl::float2> m_UVs;

    std::vector<Meshlet>                    m_meshlets;

    Resource*              VertexResource;
    Resource*              IndexResource;
    Resource*              MeshletResource;
    Resource*              MeshletTriangleIndicesResource;

    Resource*              UniqueVertexIndexResource;
    Resource*              PrimitiveIndexResource;

    int32_t m_MeshletMaxVerts = 64;
    int32_t m_MeshletMaxPrims = 124;

    MeshletizerType type = MESHOPT;

};

