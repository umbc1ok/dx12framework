#include "include/include_fullscreen.hlsl"

#ifdef SETTINGS
RasterizerDescriptor
{
    DepthClipEnable = FALSE;
    CullMode = D3D12_CULL_MODE_NONE;
}
#endif

Texture2D mainRT : register(t0);

SamplerState testSampler : register(s0);
#ifdef SAMPLER_DESC
SamplerDescriptor
{
    Name = testSampler;
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = WRAP;
    Filter = MIN_MAG_MIP_POINT;
}
#endif



float4 ps_main(VSOutput input) : SV_Target 
{
    float4 test = mainRT.Sample(testSampler, input.UV);
    //return test;
    return float4(test.x, test.y, test.z, test.w);
}