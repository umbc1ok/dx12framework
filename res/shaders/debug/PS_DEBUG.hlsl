
struct VertexOut
{
    float4 Position   : SV_Position;
    float3 PositionVS   : POSITION0;
    float3 Normal       : NORMAL0;
};


float4 ps_main(VertexOut input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 0.8f);
}