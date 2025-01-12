#include "GreedyMeshletizer.h"

#include <queue>
#include <unordered_set>


namespace meshletizers::greedy
{


    void generateMeshlets(
        const std::vector<MeshletizerVertex*>& vertsVector,
        std::vector<Meshlet>& meshlets,
        uint32_t maxVerts, uint32_t maxPrims,
        std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices,
        std::unordered_map<uint32_t,
        MeshletizerVertex*>& indexVertexMap)
    {
        std::queue<MeshletizerVertex*> priorityQueue;
        std::unordered_map<unsigned int, unsigned char> used;
        PrimitiveCache cache;
        cache.reset();

        for (int i = 0; i < vertsVector.size(); ++i)
        {
            MeshletizerVertex* vert = vertsVector[i];
            if (used.contains(vert->index))
                continue;

            priorityQueue.push(vert);


            while (!priorityQueue.empty())
            {
                MeshletizerVertex* newVertex = priorityQueue.front();
                priorityQueue.pop();

                for (Triangle* tri : newVertex->neighbours)
                {
                    if (tri->usedFlag == 1)
                        continue;

                    uint32_t candidateIndices[3] = {};
                    for (uint32_t j = 0; j < 3; ++j)
                    {
                        uint32_t idx = tri->vertices[j]->index;
                        candidateIndices[j] = idx;
                        if (!used.contains(idx))
                            priorityQueue.push(tri->vertices[j]);
                    }

                    if (cache.cannotInsert(candidateIndices, maxVerts, maxPrims))
                    {
                        for (int v = 0; v < cache.numVertices; ++v)
                        {
                            for (Triangle* newTri : indexVertexMap[cache.vertices[v]]->neighbours)
                            {
                                if (newTri->usedFlag == 1)
                                    continue;

                                for (uint32_t j = 0; j < 3; ++j)
                                {
                                    uint32_t idx = newTri->vertices[j]->index;
                                    candidateIndices[j] = idx;
                                    if (!used.contains(idx))
                                        priorityQueue.push(newTri->vertices[j]);
                                }

                                if (!cache.cannotInsert(candidateIndices, maxVerts, maxPrims))
                                {
                                    cache.insert(candidateIndices);
                                    newTri->usedFlag = 1;
                                }
                            }
                        }
                        addMeshlet(meshlets, uniqueVertexIndices, packedPrimitiveIndices, cache);

                        //reset cache and empty priorityQueue
                        priorityQueue = {};
                        priorityQueue.push(vert);
                        cache.reset();
                        continue;
                    }
                    cache.insert(candidateIndices);

                    tri->usedFlag = 1;
                }

                // TODO: CHeck why I had to comment it out
                //priorityQueue.pop();
                used[vert->index] = 1;
            }
        }
        if (!cache.empty())
        {
            addMeshlet(meshlets, uniqueVertexIndices, packedPrimitiveIndices, cache);
            cache.reset();
        }

    }

    void meshletize(
        uint32_t maxVerts, uint32_t maxPrims,
        std::vector<uint32_t>& indices,
        std::vector<Vertex>& vertices,
        std::vector<Meshlet>& meshlets,
        std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices)
    {
        std::unordered_map<uint32_t, MeshletizerVertex*> indexVertexMap;
        std::vector<Triangle*> triangles;
        std::vector<MeshletizerVertex*> vertexVector;

        generateMeshGraph(&indexVertexMap, &triangles, indices, vertices);
        sortVertices(vertices, indexVertexMap, triangles, vertexVector);

        generateMeshlets(vertexVector, meshlets, maxVerts, maxPrims, uniqueVertexIndices, packedPrimitiveIndices, indexVertexMap);
    }

}
