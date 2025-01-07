

#include <cstdint>

#include "DirectXMesh.h"
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"
#include "utils/maths.h"
#include "DX12Wrappers/Vertex.h"


namespace greedy
{
    struct PrimitiveCache;
    static const int MAX_VERTEX_COUNT_LIMIT = 64;
    static const int MAX_PRIMITIVE_COUNT_LIMIT = 124;

    struct MeshletizerVertex;
    struct Triangle
    {
        std::vector<MeshletizerVertex*> vertices;
        std::vector<Triangle*> neighbours;
        uint32_t id;
        uint32_t flag = -1;
        uint32_t dist;
    };

    struct MeshletizerVertex
    {
        std::vector<Triangle*> neighbours;
        hlsl::float3 position;
        unsigned int index;
        unsigned int degree;
    };


    void addMeshlet(
        std::vector<Meshlet>& meshlets,
        std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices,
        const PrimitiveCache& cache);

    void generateMeshGraph(
        std::unordered_map<uint32_t, MeshletizerVertex*>* indexVertexMap,
        std::vector<Triangle*>* triangles,
        const std::vector<uint32_t>& indices,
        const std::vector<Vertex>& vertices);

    void sortVertices(
        const std::vector<Vertex>& vertices,
        std::unordered_map<uint32_t, MeshletizerVertex*>& indexVertexMap,
        std::vector<Triangle*>& triangles,
        std::vector<MeshletizerVertex*>& reorganizedVertices);

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
