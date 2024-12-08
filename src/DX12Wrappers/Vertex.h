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
};

