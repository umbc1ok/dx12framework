#pragma once
#include "utils/maths.h"
#include <Windows.h>

#include "DX12Wrappers/IndexBuffer.h"
#include "DX12Wrappers/VertexBuffer.h"

class DrawableCube
{
public:
    DrawableCube();
    ~DrawableCube();

    void draw();
private:
    Vertex g_Vertices[8] = {
    { hlsl::float3(-1.0f, -1.0f, -1.0f), hlsl::float3(0.0f, 0.0f, 0.0f) }, // 0
    { hlsl::float3(-1.0f,  1.0f, -1.0f), hlsl::float3(0.0f, 1.0f, 0.0f) }, // 1
    { hlsl::float3(1.0f,  1.0f, -1.0f), hlsl::float3(1.0f, 1.0f, 0.0f) }, // 2
    { hlsl::float3(1.0f, -1.0f, -1.0f), hlsl::float3(1.0f, 0.0f, 0.0f) }, // 3
    { hlsl::float3(-1.0f, -1.0f,  1.0f), hlsl::float3(0.0f, 0.0f, 1.0f) }, // 4
    { hlsl::float3(-1.0f,  1.0f,  1.0f), hlsl::float3(0.0f, 1.0f, 1.0f) }, // 5
    { hlsl::float3(1.0f,  1.0f,  1.0f), hlsl::float3(1.0f, 1.0f, 1.0f) }, // 6
    { hlsl::float3(1.0f, -1.0f,  1.0f), hlsl::float3(1.0f, 0.0f, 1.0f) }  // 7
    };

    u16 g_Indicies[36] =
    {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

    hlsl::float4x4 m_world_matrix = {};
    VertexBuffer* m_vertex_buffer;
    IndexBuffer* m_index_buffer;
};

