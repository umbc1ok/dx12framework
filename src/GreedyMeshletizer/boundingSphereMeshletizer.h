#pragma once

#include "GreedyMeshletizer/GreedyMeshletizer.h"

namespace meshletizers::boundingSphere
{

    struct Border
    {
        Triangle* at(uint32_t index) const;
        uint32_t size() const;
        void addNeighbors(Triangle* t);
        void addTriangle(Triangle* t);
        void removeTriangle(Triangle* t);
        void clear();


        Border() {};

        std::vector<Triangle*> triangles = {};
    };

    void generateMeshlets(
        const std::vector<MeshletizerVertex*>& vertsVector,
        const std::vector<Triangle*>& triangles,
        uint32_t maxVerts, uint32_t maxPrims,
        std::vector<Meshlet>& meshlets,
        std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices,
        std::unordered_map<uint32_t,
        MeshletizerVertex*>& indexVertexMap);

    void meshletize(uint32_t maxVerts, uint32_t maxPrims, std::vector<uint32_t>& indices,
        std::vector<Vertex>& vertices, std::vector<Meshlet>& meshlets, std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices);


}