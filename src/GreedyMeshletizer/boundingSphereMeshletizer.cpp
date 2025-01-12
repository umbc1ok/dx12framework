#include "boundingSphereMeshletizer.h"

#include <queue>

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
        assert(MAX_VERTEX_COUNT_LIMIT == maxVerts);
        assert(MAX_PRIMITIVE_COUNT_LIMIT == maxPrims);
        std::unordered_map<unsigned int, unsigned char> usedVertices;

        Border border = Border();
        border.addTriangle(triangles[0]);
        PrimitiveCache cache;
        cache.reset();

        Triangle* bestTriangle = nullptr;
        float radius = 0.0f;
        hlsl::float3 center = hlsl::float3(0.0f, 0.0f, 0.0f);

        for(int i = 0; i < vertsVector.size(); i++)
        {
            MeshletizerVertex* vertex = vertsVector[i];
            //if (usedVertices.contains(vertex->index))
            //    continue;

            bestTriangle = nullptr;
            // whatever that means
            int32_t newVertex = -1;
            float newRadius = FLT_MAX;
            float bestNewRadius = FLT_MAX - 1;
            // The vertexScore for a triangle increases for each vertex that is already in
            // the meshlet and if it is considered a triangle island.
            // via: da paper
            uint32_t bestVertexScore = 0;
            // populate the border
            for (auto* t : vertex->neighbours)
            {
                if (t->usedFlag)
                    continue;
                border.addTriangle(t);
            }

            // establish which triangle is best
            for (uint32_t triangleIndex = 0; triangleIndex < border.size(); triangleIndex++)
            {
                // this is logical, that this should be here, why does the paper have it in the previous scope????
                // unless they forgot to reset it to 0 somewhere else
                uint32_t vertexScore = 0;
                auto* tri = border.at(triangleIndex);
                if (tri->usedFlag)
                    continue;

                for (uint32_t j = 0; j < 3; j++)
                {
                    if (cache.isInserted(tri->vertices[j]->index))
                        vertexScore++;
                    else
                        newVertex = tri->vertices[j]->index;
                }

                if (vertexScore == 3)
                    newRadius = radius;
                else if (vertexScore == 1)
                    continue;
                else if (newVertex > -1)
                    newRadius = 0.5f * (radius + hlsl::length(center - vertsVector[newVertex]->position));

                uint32_t trianglesInMeshlet = 0;

                for (Triangle* t : tri->neighbours)
                {
                    // Pseudo code: if tri is in Meshlet
                    // Logically this should work, if I understood the paper correctly.
                    if (t->usedFlag)
                        trianglesInMeshlet++;
                }

                // if all triangles's neighbors are in the meshlet, it should be added
                if (tri->neighbours.size() == trianglesInMeshlet)
                    vertexScore++;

                if (vertexScore >= bestVertexScore || newRadius <= bestNewRadius)
                {
                    bestVertexScore = vertexScore;
                    bestNewRadius = newRadius;
                    bestTriangle = tri;
                }
            }

            if (bestTriangle == nullptr)
            {
                for (Triangle* t : vertex->neighbours)
                {
                    if (t->usedFlag)
                        continue;
                    bestTriangle = t;
                    center = (bestTriangle->vertices[0]->position + bestTriangle->vertices[1]->position + bestTriangle->vertices[2]->position) / 3.0f;
                    float tempRadius = max(hlsl::length(center - bestTriangle->vertices[0]->position), hlsl::length(center - bestTriangle->vertices[1]->position));
                    tempRadius = max(tempRadius, hlsl::length(center - bestTriangle->vertices[2]->position));
                    bestNewRadius = tempRadius;
                }
            }

            // if we have a nullptr here it means the vertex doesn't have any unused neighbours
            if (bestTriangle == nullptr)
            {
                //i++; // not sure why this is in the pseudo code, most likely a mistake
                continue;
            }
            radius = bestNewRadius;
            center = vertsVector[newVertex]->position + (radius / (FLT_EPSILON + hlsl::length(center - vertsVector[newVertex]->position))) * (center - vertsVector[newVertex]->position);

            std::vector<uint32_t> bestTriangleIndices = { bestTriangle->vertices[0]->index, bestTriangle->vertices[1]->index , bestTriangle->vertices[2]->index };
            if(cache.cannotInsert(bestTriangleIndices.data(), maxVerts, maxPrims))
            {
                if(cache.numPrimitives < maxPrims)
                {
                    for(auto* t : border.triangles)
                    {
                        if(cache.isInserted(t->vertices[0]->index) && cache.isInserted(t->vertices[1]->index) && cache.isInserted(t->vertices[2]->index))
                        {
                            std::vector<uint32_t> t_indices = { t->vertices[0]->index,  t->vertices[1]->index,  t->vertices[2]->index };
                            if(!cache.cannotInsert(t_indices.data(), maxVerts, maxPrims))
                            {
                                t->usedFlag = true;
                                cache.insert(t_indices.data());
                                border.removeTriangle(t);

                                for (uint32_t j = 0; j < 3; j++)
                                {
                                    uint32_t vertexIndex = t->vertices[j]->index;
                                    // todo: this gives better results when commented out
                                    //usedVertices[vertexIndex] = true;
                                }
                            }
                        }
                    }
                }
                addMeshlet(meshlets, uniqueVertexIndices, packedPrimitiveIndices, cache);
                // next two are my contribution
                radius = 0;
                center = hlsl::float3(0.0f, 0.0f, 0.0f);
                cache.reset();
                border.clear();
                continue;
            }
            cache.insert(bestTriangleIndices.data());
            for (uint32_t j = 0; j < 3; j++)
            {
                uint32_t vertexIndex = bestTriangle->vertices[j]->index;
                // todo: this gives better results when commented out
                //usedVertices[vertexIndex] = true;
            }
            border.removeTriangle(bestTriangle);
            bestTriangle->usedFlag = true;
        }


        // TODO: Delete later, only here for debugging
        uint32_t used = 0;
        uint32_t unused = 0;
        for (Triangle* tri : triangles)
        {
            if (tri->usedFlag)
                used++;
            else
                unused++;
        }
        printf(" Flag statistics: Used: %d, Unused: %d\n", used, unused);
        printf("Triangles used: %llu\n", packedPrimitiveIndices.size());
        printf("Triangles target: %llu\n", triangles.size());
    }
}
