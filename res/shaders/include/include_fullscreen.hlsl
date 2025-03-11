struct VS_Input
{
    float3 Position: POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};


struct VSOutput 
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

VSOutput vs_main(VS_Input input, uint id : SV_VertexID)
{
    float2 positions[6] =
    {
        float2(-1,  1), // Top-left
        float2( 1,  1), // Top-right
        float2(-1, -1), // Bottom-left

        float2(-1, -1), // Bottom-left
        float2( 1,  1), // Top-right
        float2( 1, -1),  // Bottom-right
    };
    
    float2 uvs[6] =
    {
        float2(0, 0), // Top-left
        float2(1, 0), // Top-right
        float2(0, 1), // Bottom-left

        float2(0, 1), // Bottom-left
        float2(1, 0), // Top-right
        float2(1, 1),  // Bottom-right
    };
    
    VSOutput output;
    output.Position = float4(positions[id], 0, 1);
    output.UV = uvs[id];
    return output;
}
