
#include "meshletizerCommon.h"

namespace meshletizers::nvidia
{

    struct AdjacencyInfo
    {
        std::vector<uint32_t> trianglesPerVertex;
        std::vector<uint32_t> indexBufferOffset;
        std::vector<uint32_t> triangleData;
    };


    void tipsifyIndexBuffer(
        const std::vector<uint32_t>& indices,
        const uint32_t numVerts,
        const int cacheSize,
        std::vector<uint32_t>& optimizedIdxBuffer);

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
