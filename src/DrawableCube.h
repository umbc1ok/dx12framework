#pragma once
#include "utils/maths.h"
#include <Windows.h>

#include "Texture.h"
#include "DX12Wrappers/IndexBuffer.h"
#include "DX12Wrappers/VertexBuffer.h"

class DrawableCube
{
public:
    DrawableCube();
    ~DrawableCube();

    void draw();
private:
    //Vertex g_Vertices[8] = {
    //{ hlsl::float3(-1.0f, -1.0f, -1.0f), hlsl::float3(0.0f, 0.0f, 0.0f) }, // 0
    //{ hlsl::float3(-1.0f,  1.0f, -1.0f), hlsl::float3(0.0f, 1.0f, 0.0f) }, // 1
    //{ hlsl::float3(1.0f,  1.0f, -1.0f), hlsl::float3(1.0f, 1.0f, 0.0f) }, // 2
    //{ hlsl::float3(1.0f, -1.0f, -1.0f), hlsl::float3(1.0f, 0.0f, 0.0f) }, // 3
    //{ hlsl::float3(-1.0f, -1.0f,  1.0f), hlsl::float3(0.0f, 0.0f, 1.0f) }, // 4
    //{ hlsl::float3(-1.0f,  1.0f,  1.0f), hlsl::float3(0.0f, 1.0f, 1.0f) }, // 5
    //{ hlsl::float3(1.0f,  1.0f,  1.0f), hlsl::float3(1.0f, 1.0f, 1.0f) }, // 6
    //{ hlsl::float3(1.0f, -1.0f,  1.0f), hlsl::float3(1.0f, 0.0f, 1.0f) }  // 7
    //};
    Vertex g_Vertices[8] = {
        // Front face
        hlsl::float3(-1.0f, -1.0f, -1.0f), hlsl::float3(), hlsl::float2(0.0f, 1.0f), // 0
        hlsl::float3(-1.0f,  1.0f, -1.0f), hlsl::float3(), hlsl::float2(0.0f, 0.0f), // 1
        hlsl::float3(1.0f,  1.0f, -1.0f), hlsl::float3(), hlsl::float2(1.0f, 0.0f), // 2
        hlsl::float3(1.0f, -1.0f, -1.0f),hlsl::float3(),  hlsl::float2(1.0f, 1.0f), // 3

        // Back face
        hlsl::float3(-1.0f, -1.0f,  1.0f),hlsl::float3(), hlsl::float2(1.0f, 1.0f), // 4
        hlsl::float3(-1.0f,  1.0f,  1.0f), hlsl::float3(),hlsl::float2(1.0f, 0.0f), // 5
        hlsl::float3(1.0f,  1.0f,  1.0f),  hlsl::float3(),hlsl::float2(0.0f, 0.0f), // 6
        hlsl::float3(1.0f, -1.0f,  1.0f),  hlsl::float3(),hlsl::float2(0.0f, 1.0f)  // 7
    };

    u16 g_Indicies[36] = {
        // Front face
        0, 1, 2, 0, 2, 3,

        // Back face
        4, 6, 5, 4, 7, 6,

        // Left face
        4, 5, 1, 4, 1, 0,

        // Right face
        3, 2, 6, 3, 6, 7,

        // Top face
        1, 5, 6, 1, 6, 2,

        // Bottom face
        4, 0, 3, 4, 3, 7
    };


    hlsl::float4x4 m_world_matrix = {};
    VertexBuffer* m_vertex_buffer;
    IndexBuffer* m_index_buffer;
	Texture* m_texture;
};

