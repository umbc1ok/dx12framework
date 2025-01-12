#pragma once

#include <cstdint>

#include "DirectXMesh.h"
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"
#include "meshletizerCommon.h"


namespace meshletizers::greedy
{

    void generateMeshlets(
        const std::vector<MeshletizerVertex*>& vertsVector,
        std::vector<Meshlet>& meshlets,
        uint32_t maxVerts, uint32_t maxPrims,
        std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices,
        std::unordered_map<uint32_t,
        MeshletizerVertex*>& indexVertexMap);

    /*
     * Greedily meshletizes a mesh
     */
    void meshletize(
        uint32_t maxVerts, uint32_t maxPrims,
        std::vector<uint32_t>& indices,
        std::vector<Vertex>& vertices,
        std::vector<Meshlet>& meshlets,
        std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices);


}
