#pragma once
#include <unordered_map>

#include "utils/maths.h"
#include "DX12Wrappers/Vertex.h"

struct Meshlet;

namespace meshletizers
{
    static const int MAX_VERTEX_COUNT_LIMIT = 64;
    static const int MAX_PRIMITIVE_COUNT_LIMIT = 124;

    struct PrimitiveCache
    {
        std::vector<uint8_t> primitives;
        std::vector<uint32_t> vertices;
        uint32_t numPrimitives;
        uint32_t numVertices;

        bool empty() const;
        void reset();
        bool cannotInsert(const uint32_t* indices, uint32_t maxVertexSize, uint32_t maxPrimitiveSize) const;
        void insert(const uint32_t* indices);
        bool isInserted(uint32_t index);

        PrimitiveCache(uint32_t maxVertices, uint32_t maxPrims)
        {
            vertices.resize(maxVertices);
            primitives.resize(maxPrims * 3);
            numVertices = 0;
            numPrimitives = 0;
        }
    };


    struct MeshletizerVertex;
    struct Triangle
    {
        std::vector<MeshletizerVertex*> vertices;
        std::vector<Triangle*> neighbours;
        uint32_t id;
        bool usedFlag = 0;
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

}
