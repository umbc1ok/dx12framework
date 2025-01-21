#include "boundingSphereMeshletizer.h"

#include <queue>
#include <unordered_set>

namespace meshletizers::boundingSphere
{

    Triangle* Border::at(uint32_t index) const
    {
        return triangles.at(index);
    }

    uint32_t Border::size() const
    {
        return triangles.size();
    }
    void Border::addTriangle(Triangle* t)
    {
        triangles.push_back(t);
    }
    void Border::addNeighbors(Triangle* t)
    {
        for (auto* n : t->neighbours)
        {
            if (n->usedFlag == 0)
            {
                triangles.push_back(n);
            }
        }
    }
    void Border::removeTriangle(Triangle* t)
    {
        auto iterator = std::find(triangles.begin(), triangles.end(), t);
        if (iterator != triangles.end())
        {
            triangles.erase(iterator);
        }
    }

    void Border::clear()
    {
        triangles.clear();
    }
    


    void meshletize(uint32_t maxVerts, uint32_t maxPrims, std::vector<uint32_t>& indices,
        std::vector<Vertex>& vertices, std::vector<Meshlet>& meshlets, std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices)
    {
        std::unordered_map<uint32_t, MeshletizerVertex*> indexVertexMap;
        std::vector<Triangle*> triangles;
        std::vector<MeshletizerVertex*> vertexVector;
        generateMeshGraph(&indexVertexMap, &triangles, indices, vertices);
        sortVertices(vertices, indexVertexMap, triangles, vertexVector);
        generateMeshlets(vertexVector, triangles, maxVerts, maxPrims, meshlets, uniqueVertexIndices, packedPrimitiveIndices, indexVertexMap);
    }

    void generateMeshlets(const std::vector<MeshletizerVertex*>& vertsVector, const std::vector<Triangle*>& triangles,
        uint32_t maxVerts, uint32_t maxPrims, std::vector<Meshlet>& meshlets,
        std::vector<uint32_t>& uniqueVertexIndices, std::vector<uint32_t>& packedPrimitiveIndices,
        std::unordered_map<uint32_t, MeshletizerVertex*>& indexVertexMap)
    {

        std::unordered_map<unsigned int, unsigned char> usedVerts;
        std::unordered_set<uint32_t> currentVerts;
        float radius = .0f;
        hlsl::float3 center = hlsl::float3(0.0f, 0.0f, 0.0f);
        PrimitiveCache cache(maxVerts, maxPrims);
        cache.reset();

        for (int i = 0; i < vertsVector.size();)
        {
            MeshletizerVertex* vert = vertsVector[i];
            Triangle* bestTri = nullptr;
            float newRadius = FLT_MAX;
            float bestNewRadius = FLT_MAX - 1.0f;
            int bestVertsInMeshlet = 0;

            for (uint32_t j = 0; j < cache.numVertices; ++j)
            {
                uint32_t vertId = cache.vertices[j];

                for (Triangle* tri : indexVertexMap[vertId]->neighbours)
                {
                    if (tri->usedFlag == 1) continue;

                    // get info about tri
                    int newVert{};
                    int vertsInMeshlet = 0;
                    int used = 0;
                    for (int i = 0; i < 3; ++i)
                    {
                        if (currentVerts.find(tri->vertices[i]->index) == currentVerts.end())
                        {
                            newVert = i;
                        }
                        else
                        {
                            ++vertsInMeshlet;
                        }
                    }

                    for (auto neighbour_tri : tri->neighbours)
                    {
                        if (neighbour_tri->usedFlag == 1) ++used;
                    }

                    if (tri->neighbours.size() == used)
                        used = 3;


                    // if dangling triangle add it
                    if (used == 3)
                    {
                        ++vertsInMeshlet;
                    }

                    //if all verts are allready in meshlet
                    if (vertsInMeshlet == 3)
                    {
                        newRadius = radius;
                    }
                    else if (vertsInMeshlet == 1)
                    {
                        continue;
                    }
                    else 
                    {
                        float newRadius = 0.5 * (radius + hlsl::length(center - tri->vertices[newVert]->position));
                    }

                    if (vertsInMeshlet > bestVertsInMeshlet || newRadius < bestNewRadius) {
                        bestVertsInMeshlet = vertsInMeshlet;
                        bestNewRadius = newRadius;
                        bestTri = tri;
                    }
                }
            }

            if (bestTri == nullptr)
            {
                // create radius and center for the first triangle in the meshlet
                for (Triangle* tri : vert->neighbours) {
                    // skip used triangles
                    if (tri->usedFlag != 1) {
                        bestTri = tri;

                        center = (bestTri->vertices[0]->position + bestTri->vertices[1]->position + bestTri->vertices[2]->position) / 3.0f;
                        float eins = hlsl::length(center - bestTri->vertices[0]->position);
                        float zwei = hlsl::length(center - bestTri->vertices[1]->position);
                        float drei = hlsl::length(center - bestTri->vertices[2]->position);
                        bestNewRadius = std::max(eins, std::max(zwei, drei));
                        break;
                    }
                }

                if (bestTri == nullptr)
                {
                    ++i;
                    continue;
                }
            }

            int newVert{};
            int numNewVerts = 0;
            uint32_t candidateIndices[3];
            for (uint32_t i = 0; i < 3; ++i) {
                candidateIndices[i] = bestTri->vertices[i]->index;
                if (currentVerts.find(bestTri->vertices[i]->index) == currentVerts.end()) {
                    newVert = i;
                    ++numNewVerts;
                }
            }

            radius = bestNewRadius;
            if (numNewVerts == 1)
            {
                // get all vertices of current triangle
                const hlsl::float3 position = bestTri->vertices[newVert]->position;
                center = position + (radius / (FLT_EPSILON + hlsl::length(center - position))) * (center - position);
            }

            // If full pack and restart restart
            // add triangle to cache
            if (cache.cannotInsert(candidateIndices, maxVerts, maxPrims)) {
                // we run out of verts but could push prims more so we do a pass of prims here to see if we can maximize 
                // so we run through all triangles to see if the meshlet already has the required verts
                // we try to do this in a dum way to test if it is worth it
                for (int v = 0; v < cache.numVertices; ++v) {
                    for (Triangle* tri : indexVertexMap[cache.vertices[v]]->neighbours) {
                        if (tri->usedFlag == 1) continue;

                        uint32_t candidateIndices[3];
                        for (uint32_t j = 0; j < 3; ++j) {
                            uint32_t idx = tri->vertices[j]->index;
                            candidateIndices[j] = idx;
                        }

                        if (!cache.cannotInsert(candidateIndices, maxVerts, maxPrims)) {
                            cache.insert(candidateIndices);
                            tri->usedFlag = 1;
                        }
                    }
                }
                addMeshlet(meshlets, uniqueVertexIndices, packedPrimitiveIndices, cache);
                currentVerts.clear();
                cache.reset();
                continue;
                //break;


            }

            // insert triangle and mark used
            cache.insert(candidateIndices);
            bestTri->usedFlag = 1;
            currentVerts.insert(candidateIndices[0]);
            currentVerts.insert(candidateIndices[1]);
            currentVerts.insert(candidateIndices[2]);
            ++usedVerts[candidateIndices[0]];
            ++usedVerts[candidateIndices[1]];
            ++usedVerts[candidateIndices[2]];

        }

        // add remaining triangles to a meshlet
        if (!cache.empty()) {
            addMeshlet(meshlets, uniqueVertexIndices, packedPrimitiveIndices, cache);
            cache.reset();
        }

    }
}
