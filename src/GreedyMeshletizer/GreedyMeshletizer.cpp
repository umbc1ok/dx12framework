#include "GreedyMeshletizer.h"

#include <unordered_set>

#include "utils/Utils.h"

namespace greedy
{

    struct PrimitiveCache
    {
        uint8_t primitives[MAX_PRIMITIVE_COUNT_LIMIT * 3];
        uint32_t vertices[MAX_VERTEX_COUNT_LIMIT];
        uint32_t numPrimitives;
        uint32_t numVertices;

        bool empty() const { return numVertices == 0; }

        void reset()
        {
            numPrimitives = 0;
            numVertices = 0;
            // reset
            memset(vertices, 0xFFFFFFFF, sizeof(vertices));
        }

        bool cannotInsert(const uint32_t* indices, uint32_t maxVertexSize, uint32_t maxPrimitiveSize) const
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

        void insert(const uint32_t* indices)
        {
            uint32_t tri[3];

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
    };

    void addMeshlet(std::vector<Meshlet>& meshlets,
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


    void meshletize(
        uint32_t maxVerts, uint32_t maxPrims,
        const std::vector<uint32_t>& indices, uint32_t indexCount,
        std::vector<Meshlet>& meshlets,
        std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices)
    {
        PrimitiveCache cache;
        cache.reset();

        std::unordered_map<uint32_t, Vertex*> indexVertexMap;
        std::vector<Triangle*> triangles;

        generateMeshGraph(&indexVertexMap, &triangles, indices.size(), indices);

        std::vector<bool> used(triangles.size(), false);

        std::unordered_set<uint32_t> currentVerts;

        uint32_t score;
        uint32_t maxScore;


        for(uint32_t usedCount = 0 ; usedCount < triangles.size(); usedCount++)
        {
            if (used[usedCount]) continue;

            std::vector<Triangle*> frontier = { triangles[usedCount] };
            currentVerts.clear();

            while(frontier.size() > 0)
            {
                maxScore = 0;

                Triangle* candidate;
                uint32_t candidateIndex;
                uint32_t candidateIndices[3];

                for (uint32_t i = 0; i < frontier.size(); i++)
                {
                    Triangle* current = frontier[i];
                    score = 0;
                    for (Vertex* v : current->vertices)
                        score += currentVerts.count(v->index);

                    if (score >= maxScore)
                    {
                        maxScore = score;
                        candidate = current;
                        candidateIndex = i;
                    }
                }
                for (uint32_t i = 0; i < 3; ++i)
                    candidateIndices[i] = candidate->vertices[i]->index;

                // check if we've filled the meshlet already
                if (cache.cannotInsert(candidateIndices, maxVerts, maxPrims))
                {
                    addMeshlet(meshlets, uniqueVertexIndices, packedPrimitiveIndices, cache);
                    cache.reset();
                    break;
                }

                cache.insert(candidateIndices);
                std::swap(frontier[candidateIndex], frontier[frontier.size() - 1]);
                frontier.pop_back();

                for (Vertex* v : candidate->vertices) 
                    currentVerts.insert(v->index);
                for (Triangle* t : candidate->neighbours)
                {
                    if (!used[t->id]) frontier.push_back(t);
                }
                used[candidate->id] = true;
            }
            if (!cache.empty())
            {
                addMeshlet(meshlets, uniqueVertexIndices, packedPrimitiveIndices, cache);
            }
        }

    }

    void generateMeshGraph(
        std::unordered_map<uint32_t, Vertex*>* indexVertexMap,
        std::vector<Triangle*>* triangles,
        const uint32_t numIndices,
        const std::vector<uint32_t>& indices)
    {
        int unique = 0;
        int reused = 0;

        triangles->resize(numIndices / 3);
        for(int i = 0; i < numIndices / 3 ; i++)
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
                    Vertex* v = new Vertex();
                    v->index = indices[i * 3 + j];
                    v->degree = 1;
                    v->neighbours.push_back(t);
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
        Vertex* currentVertex;
        // possibly rename to something more descriptive
        Vertex* connectedVertex;

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




}
