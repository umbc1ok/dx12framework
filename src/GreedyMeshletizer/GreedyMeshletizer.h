

#include <cstdint>

#include "DirectXMesh.h"
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"
#include "utils/maths.h"

namespace greedy
{
    static const int MAX_VERTEX_COUNT_LIMIT = 64;
    static const int MAX_PRIMITIVE_COUNT_LIMIT = 124;

    struct Vertex;
    struct Triangle
    {
        std::vector<Vertex*> vertices;
        std::vector<Triangle*> neighbours;
        uint32_t id;
        uint32_t flag = -1;
        uint32_t dist;
    };

    struct Vertex
    {
        std::vector<Triangle*> neighbours;
        unsigned int index;
        unsigned int degree;
    };

    // needed:
    // in: indices, vertices, positions
    // out: meshlets, unique_vertex_indices (uint8_t, primitive_indices (uint8_t indices insiede a meshlet)
    void meshletize(
        uint32_t maxVerts, uint32_t maxPrims,
        const std::vector<uint32_t>& indices, uint32_t indexCount,
        std::vector<Meshlet>& meshlets,
        std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices);

    void generateMeshGraph(
        std::unordered_map<uint32_t, Vertex*>* indexVertexMap,
        std::vector<Triangle*>* triangles,
        const uint32_t numIndices,
        const std::vector<uint32_t>& indices);
}
