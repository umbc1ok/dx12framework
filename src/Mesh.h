#pragma once
#include <vector>


#include "DX12Wrappers/IndexBuffer.h"
#include "DX12Wrappers/Vertex.h"
#include "DX12Wrappers/VertexBuffer.h"
#include "Texture.h"
#include <d3d12.h>
#include <wrl/client.h>
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
/*

struct Subset
{
    uint32_t Offset;
    uint32_t Count;
};


struct Meshlet
{
    uint32_t VertCount;
    uint32_t VertOffset;
    uint32_t PrimCount;
    uint32_t PrimOffset;
};

struct PackedTriangle
{
    uint32_t i0 : 10;
    uint32_t i1 : 10;
    uint32_t i2 : 10;
};

struct CullData
{
    hlsl::float4 BoundingSphere; // xyz = center, w = radius
    uint8_t           NormalCone[4];  // xyz = axis, w = -cos(a + 90)
    float             ApexOffset;     // apex = center - axis * offset
};
*/
inline uint32_t AddAttribute(uint32_t base, Attribute::EType add) { return base | (1 << add); }

class Mesh
{
public:
    Mesh(std::vector<Vertex> const& vertices, std::vector<u16> const& indices, std::vector<Texture*> const& textures, std::vector<hlsl::float3> const& positions, std::vector<hlsl::float3> const& normals, std::vector<hlsl::float2> const& UVS, std::vector<u32> attributes);
    ~Mesh() = default;

    void draw();
    void bind_textures();
    void dispatch();

    void meshletize();
    std::vector<Vertex> m_vertices;
    std::vector<u16> m_indices;
    std::vector<Texture*> m_textures;

    std::vector<uint32_t>              m_vertexRemap;
    std::vector<uint16_t>              m_indexReorder;
    std::vector<uint32_t>              m_dupVerts;

    std::vector<hlsl::float3>          m_positionReorder;
    std::vector<hlsl::float3>          m_normalReorder;
    std::vector<hlsl::float2>          m_uvReorder;

    std::vector<uint32_t>              m_faceRemap;

    std::vector<uint32_t>               m_attributes;

    std::vector<hlsl::float3> m_positions;
    std::vector<hlsl::float3> m_normals;
    std::vector<hlsl::float2> m_UVs;

    std::vector<Subset>                     m_meshletSubsets;
    std::vector<Meshlet>                    m_meshlets;
    std::vector<uint8_t>                    m_uniqueVertexIndices;
    std::vector<PackedTriangle>             m_primitiveIndices;
    std::vector<CullData>                   m_cullData;
    std::vector<Subset>                     m_indexSubsets;

    std::vector<hlsl::float3>          m_tangents;
    std::vector<hlsl::float3>          m_bitangents;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> VertexResources;
    Microsoft::WRL::ComPtr<ID3D12Resource>              IndexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              MeshletResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              UniqueVertexIndexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              PrimitiveIndexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              CullDataResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              MeshInfoResource;

    std::vector<D3D12_VERTEX_BUFFER_VIEW>  VBViews;
    D3D12_INDEX_BUFFER_VIEW                IBView;

    MeshInfo mesh_info;
    int32_t MeshletMaxVerts = 64;
    int32_t MeshletMaxPrims = 126;
private:
    VertexBuffer* m_vertex_buffer;
    IndexBuffer* m_index_buffer;


};

