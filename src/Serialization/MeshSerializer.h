#pragma once


#include "MeshletStructs.h"
#include "DX12Wrappers/Vertex.h"
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"
#include "types/VectorSerializer.h"
#include "utils/maths.h"


namespace serializers
{
    bool serializeMesh(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::vector<Meshlet>& meshlets,
        const std::vector<uint32_t>& meshletTriangles,
        const std::vector<uint32_t>& attributes,
        const std::vector<hlsl::float3>& positions,
        const std::vector<hlsl::float3>& normals,
        const std::vector<hlsl::float2>& UVs,
        const std::vector<CullData>& cullData,
        int32_t MeshletMaxVerts,
        int32_t MeshletMaxPrims,
        MeshletizerType type,
        const std::string& fileName);

    void deserializeMesh(
         std::vector<Vertex>& vertices,
         std::vector<uint32_t>& indices,
         std::vector<Meshlet>& meshlets,
         std::vector<uint32_t>& meshletTriangles,
         std::vector<uint32_t>& attributes,
         std::vector<hlsl::float3>& positions,
         std::vector<hlsl::float3>& normals,
         std::vector<hlsl::float2>& UVs,
         std::vector<CullData>& cullData,
         int32_t & MeshletMaxVerts,
         int32_t & MeshletMaxPrims,
         MeshletizerType& type,
         const std::string& fileName);

}
