#include "../shared/shared_debug_cb.h"

struct VS_Input
{
    float3 Position: POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

struct VertexOut
{
    float4 Position     : SV_Position;
    float3 PositionVS   : POSITION0;
    float3 Normal       : NORMAL0;
};


ConstantBuffer<TransformationMatrices> transforms       : register(b0);


VertexOut vs_main(VS_Input input)
{
    VertexOut output;
    //output.PositionVS = mul(float4(input.Position, 1), transforms.WorldView).xyz;
    output.Position = mul(float4(input.Position, 1), transforms.WorldViewProj).xyzw;
    output.Normal = mul(float4(input.Normal, 0), transforms.World).xyz;

    return output;
}
