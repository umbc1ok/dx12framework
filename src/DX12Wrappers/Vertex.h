#pragma once
#include "utils/maths.h"

struct Vertex
{
    hlsl::float3 position;
    hlsl::float3 normal;
    hlsl::float2 UV;

    Vertex() {};
    Vertex(hlsl::float3 pos, hlsl::float3 norm, hlsl::float2 uv) : position(pos), normal(norm), UV(uv) {}
};

struct Vertex2
{
    hlsl::float3 position;
    hlsl::float3 normal;
    hlsl::float2 UV;

    Vertex2() {};
    Vertex2(hlsl::float3 pos, hlsl::float3 norm, hlsl::float2 uv) : position(pos), normal(norm), UV(uv) {}
};
