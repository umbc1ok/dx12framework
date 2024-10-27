// Toy engine @ 2024
// Author : Hubert Olejnik 

#include "shared/shared_cb.h"

struct VertexOut
{
    float4 PositionHS   : SV_Position;
    float3 PositionVS   : POSITION0;
    float3 Normal       : NORMAL0;
    float   stiffness : COLOR0;
};

ConstantBuffer<SceneConstantBuffer> Globals : register(b0);


#include "shared/shared_cb.h"

float4 ps_main(VertexOut input) : SV_TARGET
{
    float g = input.stiffness;
    g = clamp(g, 0.3f, 1.0f);
    return float4(0.1, g, 0.1f, 1.0f);
}
