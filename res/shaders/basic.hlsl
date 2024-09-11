
struct ModelViewProjection
{
    float4x4 MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

Texture2D foo : register(t0);
SamplerState test_sampler : register(s0);

struct VertexPosColor
{
    float3 Position : POSITION;
    float2 UV    : TEXCOORD;
};

struct VertexShaderOutput
{
    float2 UV    : TEXCOORD;
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
    OUT.UV = float2(IN.UV);
 
    return OUT;
}


 
float4 ps_main( VertexShaderOutput IN ) : SV_Target
{
    return foo.Sample(test_sampler, IN.UV);
}
