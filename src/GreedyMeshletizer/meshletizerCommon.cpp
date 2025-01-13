#include "meshletizerCommon.h"

#include <iostream>

#include "Mesh.h"


#include "utils/Utils.h"

namespace meshletizers
{


    bool PrimitiveCache::empty() const { return numVertices == 0; }

    void PrimitiveCache::reset()
    {
        numPrimitives = 0;
        numVertices = 0;
        // reset
        for (int i = 0; i < vertices.size(); i++)
        {
            vertices[i] = UINT32_MAX;
        }
        for (int i = 0; i < primitives.size(); i++)
        {
            primitives[i] = UINT8_MAX;
        }
    }

    bool PrimitiveCache::cannotInsert(const uint32_t* indices, uint32_t maxVertexSize, uint32_t maxPrimitiveSize) const
    {
        // skip degenerate
        if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
        {
            return false;
        }

        uint32_t found = 0;

        for (uint32_t v = 0; v < numVertices; v++)
        {
            for (int i = 0; i < 3; i++)
            {
                uint32_t idx = indices[i];
                if (vertices[v] == idx)
                {
                    found++;
                }
            }
        }
        // out of bounds
        return (numVertices + 3 - found) > maxVertexSize || (numPrimitives + 1) > maxPrimitiveSize;
    }

    void PrimitiveCache::insert(const uint32_t* indices)
    {
        uint8_t tri[3];

        // skip degenerate
        if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
        {
            return;
        }

        for (int i = 0; i < 3; i++)
        {
            uint32_t idx = indices[i];
            bool     found = false;
            for (uint32_t v = 0; v < numVertices; v++)
            {
                if (idx == vertices[v])
                {
                    tri[i] = v;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                vertices[numVertices] = idx;
                tri[i] = numVertices;
                numVertices++;
            }
        }

        primitives[numPrimitives * 3] = tri[0];
        primitives[numPrimitives * 3 + 1] = tri[1];
        primitives[numPrimitives * 3 + 2] = tri[2];
        numPrimitives++;
    }

    bool PrimitiveCache::isInserted(uint32_t index)
    {
        for (uint32_t v = 0; v < numVertices; v++)
        {
            if (vertices[v] == index)
            {
                return true;
            }
        }
        return false;
    }

    void addMeshlet(
        std::vector<Meshlet>& meshlets,
        std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices,
        const PrimitiveCache& cache)
    {
        Meshlet meshlet;
        meshlet.VertCount = cache.numVertices;
        meshlet.PrimCount = cache.numPrimitives;
        meshlet.VertOffset = meshlets.empty() ? 0 : meshlets.back().VertOffset + meshlets.back().VertCount;
        meshlet.PrimOffset = meshlets.empty() ? 0 : meshlets.back().PrimOffset + meshlets.back().PrimCount;
        meshlets.push_back(meshlet);

        for (int i = 0; i < cache.numVertices; i++)
        {
            uniqueVertexIndices.push_back(cache.vertices[i]);
        }

        for (int i = 0; i < cache.numPrimitives; i++)
        {
            packedPrimitiveIndices.push_back(olej_utils::packTriangle(cache.primitives[i * 3 + 0], cache.primitives[i * 3 + 1], cache.primitives[i * 3 + 2]));
        }
    }

    void generateMeshGraph(
        std::unordered_map<uint32_t, MeshletizerVertex*>* indexVertexMap,
        std::vector<Triangle*>* triangles,
        const std::vector<uint32_t>& indices,
        const std::vector<Vertex>& vertices)
    {
        int unique = 0;
        int reused = 0;

        triangles->resize(indices.size() / 3);
        for (int i = 0; i < indices.size() / 3; i++)
        {
            Triangle* t = new Triangle();
            t->id = i;
            (*triangles)[i] = t;
            for (int j = 0; j < 3; j++)
            {
                auto lookup = indexVertexMap->find(indices[i * 3 + j]);
                if (lookup != indexVertexMap->end())
                {
                    lookup->second->neighbours.push_back(t);
                    lookup->second->degree++;
                    t->vertices.push_back(lookup->second);
                    reused++;
                }
                else
                {
                    MeshletizerVertex* v = new MeshletizerVertex();
                    v->index = indices[i * 3 + j];
                    v->degree = 1;
                    v->neighbours.push_back(t);
                    v->position = vertices[v->index].position;
                    (*indexVertexMap)[v->index] = v;
                    t->vertices.push_back(v);
                    unique++;
                }
            }
        }

        // Connect vertices
        uint32_t found;
        Triangle* currentTriangle;
        Triangle* candidateNeighborTriangle;
        MeshletizerVertex* currentVertex;
        // possibly rename to something more descriptive
        MeshletizerVertex* connectedVertex;

        // Find adjacent triangles
        for (uint32_t i = 0; i < triangles->size(); ++i)
        {
            currentTriangle = (*triangles)[i];
            found = 0;
            for (uint32_t j = 0; j < 3; ++j)
            {
                currentVertex = currentTriangle->vertices[j];
                // For each triangle containing each vertex of each triangle
                for (uint32_t k = 0; k < currentVertex->neighbours.size(); ++k)
                {
                    candidateNeighborTriangle = currentVertex->neighbours[k];
                    if (candidateNeighborTriangle->id == currentTriangle->id) continue; // You are yourself a neighbour of your neighbours
                    // For each vertex of each triangle containing ...
                    for (uint32_t l = 0; l < 3; ++l)
                    {
                        connectedVertex = candidateNeighborTriangle->vertices[l];
                        if (connectedVertex->index == currentTriangle->vertices[(j + 1) % 3]->index)
                        {
                            found++;
                            currentTriangle->neighbours.push_back(candidateNeighborTriangle);
                            break;
                        }
                    }
                }
            }
        }
    }


    void sortVertices(
        const std::vector<Vertex>& vertices,
        std::unordered_map<uint32_t, MeshletizerVertex*>& indexVertexMap,
        std::vector<Triangle*>& triangles,
        std::vector<MeshletizerVertex*>& reorganizedVertices)
    {
        hlsl::float3 min{ FLT_MAX, FLT_MAX, FLT_MAX };
        hlsl::float3 max{ FLT_MIN, FLT_MIN, FLT_MIN };

        for (Triangle* tri : triangles)
        {
            min = hlsl::min(min, indexVertexMap[tri->vertices[0]->index]->position);
            min = hlsl::min(min, indexVertexMap[tri->vertices[1]->index]->position);
            min = hlsl::min(min, indexVertexMap[tri->vertices[2]->index]->position);

            max = hlsl::max(max, indexVertexMap[tri->vertices[0]->index]->position);
            max = hlsl::max(max, indexVertexMap[tri->vertices[1]->index]->position);
            max = hlsl::max(max, indexVertexMap[tri->vertices[2]->index]->position);
        }

        hlsl::float3 axis = hlsl::abs(max - min);

        reorganizedVertices.reserve(indexVertexMap.size());
        for (int i = 0; i < indexVertexMap.size(); i++)
        {
            reorganizedVertices.push_back(indexVertexMap[i]);
        }

        // Sort by X axis
        if (axis.x > axis.y && axis.x > axis.z)
        {
            std::sort(reorganizedVertices.begin(), reorganizedVertices.end(),
                [&](const MeshletizerVertex* a, const MeshletizerVertex* b)
                {
                    return vertices[a->index].position[0] > vertices[b->index].position[0];
                });

            std::cout << "Sorted by X axis" << std::endl;
        }
        // Sort by Y axis
        else if (axis.y > axis.z && axis.y > axis.x)
        {
            std::sort(reorganizedVertices.begin(), reorganizedVertices.end(),
                [&](const MeshletizerVertex* a, const MeshletizerVertex* b)
                {
                    return vertices[a->index].position[1] > vertices[b->index].position[1];
                });

            std::cout << "Sorted by Y axis" << std::endl;
        }
        // Sort by Z axis
        else
        {
            std::sort(reorganizedVertices.begin(), reorganizedVertices.end(),
                [&](const MeshletizerVertex* a, const MeshletizerVertex* b)
                {
                    return vertices[a->index].position[2] > vertices[b->index].position[2];
                });

            std::cout << "Sorted by Z axis" << std::endl;
        }
    }
}