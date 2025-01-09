#pragma once
#include "utils/maths.h"

struct Vertex
{
    hlsl::float3 position;
    float padding;
    hlsl::float3 normal;
    float padding2;
    hlsl::float2 UV;
    hlsl::float2 padding3;

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
