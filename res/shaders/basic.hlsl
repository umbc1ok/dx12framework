
struct ModelViewProjection
{
    float4x4 MVP;
}

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Color    : COLOR;
};
struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};


struct PixelShaderInput
{
    float4 Color    : COLOR;
};



VertexShaderOutput vs_main(VertexPosColor IN)
{
    VertexShaderOutput OUT;
 
    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    OUT.Color = float4(IN.Color, 1.0f);
 
    return OUT;
}


 
float4 main( PixelShaderInput IN ) : SV_Target
{
    return IN.Color;
}
