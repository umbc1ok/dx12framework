#include "nvMeshletizer.h"

#include <cassert>
#include <queue>

namespace meshletizers::nvidia
{

	// faster make mesh function
	void buildAdjacency(const uint32_t numVerts, const uint32_t numIndices, const std::vector<uint32_t>& indices, AdjacencyInfo& info) {
		// we loop over index buffer and count now often a vertex is used
		info.trianglesPerVertex.resize(numVerts, 0);
		for (int i = 0; i < numIndices; ++i) {
			info.trianglesPerVertex[indices[i]]++;
		}

		//  save the offsets needed to look up into the index buffer for a given triangle
		uint32_t triangleOffset = 0;
		info.indexBufferOffset.resize(numVerts, 0);
		for (int j = 0; j < numVerts; ++j) {
			info.indexBufferOffset[j] = triangleOffset;
			triangleOffset += info.trianglesPerVertex[j];
		}


		// save triangle information
		uint32_t numTriangles = numIndices / 3;
		info.triangleData.resize(triangleOffset);
		std::vector<uint32_t> offsets = info.indexBufferOffset;
		for (uint32_t k = 0; k < numTriangles; ++k) {
			int a = indices[k * 3];
			int b = indices[k * 3 + 1];
			int c = indices[k * 3 + 2];

			info.triangleData[offsets[a]++] = k;
			info.triangleData[offsets[b]++] = k;
			info.triangleData[offsets[c]++] = k;
		}
	}

	uint32_t skipDeadEnd(const std::vector<uint32_t>& indices, const std::vector<uint32_t>& liveTriCount, std::queue<uint32_t>& deadEndStack, const uint32_t& curVert, const uint32_t& numVerts, uint32_t& cursor) {
		while (!deadEndStack.empty()) {
			uint32_t vertIdx = deadEndStack.front();
			deadEndStack.pop();
			if (liveTriCount[vertIdx] > 0) {
				return vertIdx;
			}
		}
		while (cursor < liveTriCount.size()) {
			if (liveTriCount[cursor] > 0) {
				return cursor;
			}
			++cursor;
		}

		return -1;
	}

	uint32_t getNextVertex(const std::vector<uint32_t>& indices, const uint32_t& curVert, const int& cacheSize, const std::vector<uint32_t>& oneRing, const std::vector<uint32_t>& cacheTimeStamps, const uint32_t& timeStamp, const std::vector<uint32_t>& liveTriCount, std::queue<uint32_t>& deadEndStack, const uint32_t& numVerts, uint32_t& curser) {
		uint32_t bestCandidate = -1;
		int highestPriority = -1;

		for (const uint32_t& vertIdx : oneRing) {
			if (liveTriCount[vertIdx] > 0) {
				int priority = 0;
				if (timeStamp - cacheTimeStamps[vertIdx] + 2 * liveTriCount[vertIdx] <= cacheSize)
				{
					priority = timeStamp - cacheTimeStamps[vertIdx];
				}
				if (priority > highestPriority)
				{
					highestPriority = priority;
					bestCandidate = vertIdx;
				}
			}
		}

		if (bestCandidate == -1)
		{
			bestCandidate = skipDeadEnd(indices, liveTriCount, deadEndStack, curVert, numVerts, curser);
		}
		return bestCandidate;
	}


    void generateMeshlets(const std::vector<uint32_t>& indices, std::vector<Meshlet>& meshlets,
        uint32_t maxVerts, uint32_t maxPrims, std::vector<uint32_t>& uniqueVertexIndices,
        std::vector<uint32_t>& packedPrimitiveIndices)
    {

        PrimitiveCache cache = PrimitiveCache(maxVerts, maxPrims);
        cache.reset();

        for (uint32_t i = 0; i < indices.size() / 3; i++)
        {
            uint32_t candidateIndices[3] = { indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2] };
            if (cache.cannotInsert(candidateIndices, maxVerts, maxPrims))
            {
                // finish old and reset
                addMeshlet(meshlets, uniqueVertexIndices, packedPrimitiveIndices, cache);
                cache.reset();
            }
            cache.insert(candidateIndices);
        }
        if (!cache.empty())
        {
            addMeshlet(meshlets,uniqueVertexIndices,packedPrimitiveIndices, cache);
        }
    }

    void tipsifyIndexBuffer(const std::vector<uint32_t>& indices, const uint32_t numVerts, const int cacheSize,
        std::vector<uint32_t>& optimizedIdxBuffer)
    {
		AdjacencyInfo adjacencyStruct;
		buildAdjacency(numVerts, indices.size(), indices, adjacencyStruct);

		// create a copy of the triangle per vertex count
		std::vector<uint32_t> liveTriCount = adjacencyStruct.trianglesPerVertex;

		// per vertex caching time stamp
		std::vector<uint32_t> cacheTimeStamps(numVerts);

		// stack to keep track of dead-end verts
		std::queue<uint32_t> deadEndStack;

		// keep track of emitted triangles
		std::vector<bool> emittedTriangles(indices.size() / 3, false);

		//new index buffer
		//std::vector<uint32_t> optimizedIdxBuffer;


		uint32_t curVert = 0; // Arbitrary starting vertex
		uint32_t timeStamp = cacheSize + 1; // time stap
		uint32_t cursor = 1; // to keep track of next vertex index

		while (curVert != -1) {
			// for 1 ring of current vert
			std::vector<uint32_t> oneRing;

			// find starting tiangle and num triangles
			const uint32_t* startTriPointer = &adjacencyStruct.triangleData[0] + adjacencyStruct.indexBufferOffset[curVert];
			const uint32_t* endTriPointer = startTriPointer + adjacencyStruct.trianglesPerVertex[curVert];

			const uint32_t startTri = adjacencyStruct.triangleData[0] + adjacencyStruct.indexBufferOffset[curVert];
			const uint32_t endTri = startTri + adjacencyStruct.trianglesPerVertex[curVert];

			for (const uint32_t* it = startTriPointer; it != endTriPointer; ++it)
			{

				uint32_t triangle = *it;

				if (emittedTriangles[triangle])
					continue;

				// find vertex indices for current triangle
				uint32_t a = indices[triangle * 3];
				uint32_t b = indices[triangle * 3 + 1];
				uint32_t c = indices[triangle * 3 + 2];

				// add triangle to out index buffer
				optimizedIdxBuffer.push_back(a);
				optimizedIdxBuffer.push_back(b);
				optimizedIdxBuffer.push_back(c);

				// add indices to dead end stack
				deadEndStack.push(a);
				deadEndStack.push(b);
				deadEndStack.push(c);

				// add indices to dead end stack
				oneRing.push_back(a);
				oneRing.push_back(b);
				oneRing.push_back(c);

				liveTriCount[a]--;
				liveTriCount[b]--;
				liveTriCount[c]--;

				if (timeStamp - cacheTimeStamps[a] > cacheSize) {
					cacheTimeStamps[a] = timeStamp;
					++timeStamp;
				}

				if (timeStamp - cacheTimeStamps[b] > cacheSize) {
					cacheTimeStamps[b] = timeStamp;
					++timeStamp;
				}

				if (timeStamp - cacheTimeStamps[c] > cacheSize) {
					cacheTimeStamps[c] = timeStamp;
					++timeStamp;
				}

				emittedTriangles[triangle] = true;
			}

			curVert = getNextVertex(indices, curVert, cacheSize, oneRing, cacheTimeStamps, timeStamp, liveTriCount, deadEndStack, numVerts, cursor);
		}
    }

    void meshletize(uint32_t maxVerts, uint32_t maxPrims, std::vector<uint32_t>& indices, std::vector<Vertex>& vertices,
                    std::vector<Meshlet>& meshlets, std::vector<uint32_t>& uniqueVertexIndices,
                    std::vector<uint32_t>& packedPrimitiveIndices)
    {
		std::vector<uint32_t> optimizedIndices;
        tipsifyIndexBuffer(indices, vertices.size(), 32, optimizedIndices);
        generateMeshlets(optimizedIndices, meshlets, maxVerts, maxPrims, uniqueVertexIndices, packedPrimitiveIndices);
    }
}
