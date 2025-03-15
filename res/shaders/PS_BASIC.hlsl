//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
#include "shared/shared_cb.h"

struct VertexOut
{
    float4 PositionHS   : SV_Position;
    float3 PositionVS   : POSITION0;
    float3 Normal       : NORMAL0;
    float2 UV : TEXCOORD0;
    uint   MeshletIndex : COLOR0;
    uint   TriangleIndex : COLOR1;
};

#ifdef SETTINGS
RasterizerDescriptor
{
    FillMode = FILL;
	CullMode = BACK;
	FrontCounterClockwise = TRUE;
	MSEnable = TRUE;
	AntialiasedLineEnable = TRUE;
	ConservativeRaster = FALSE;
}
#endif

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

ConstantBuffer<SceneConstantBuffer> InstanceData : register(b0);

Texture2D DIFFUSE_TEX : register(t0);



float4 ps_main(VertexOut input) : SV_TARGET
{
    //return 1.0f.xxxx;
    float4 tex =  DIFFUSE_TEX.Sample(testSampler, input.UV);
    float ambientIntensity = 0.1;
    float3 lightColor = float3(1, 1, 1);
    float3 lightDir = -normalize(float3(1, -1, 1));

    float3 diffuseColor;
    float shininess;
    if(tex.a < 0.9) discard;
    if (InstanceData.DrawFlag == DRAW_MESHLETS)
    {
        uint index = input.MeshletIndex;
        diffuseColor = float3(
            float(index & 1),
            float(index & 3) / 4,
            float(index & 7) / 8);
        shininess = 16.0;
    }
    else if(InstanceData.DrawFlag == DRAW_TRIANGLES)
    {
        uint index = input.TriangleIndex;
        diffuseColor = float3(
            float(index & 1),
            float(index & 3) / 4,
            float(index & 7) / 8);
        shininess = 16.0;
    }
    else
    {
        diffuseColor = tex.xyz;
        shininess = 64.0;
    }
    float3 finalColor;
    if(InstanceData.DrawFlag == DRAW_NORMAL)
    {
        float3 normal = normalize(input.Normal);

        // Do some fancy Blinn-Phong shading!
        float cosAngle = saturate(dot(normal, lightDir));
        float3 viewDir = -normalize(input.PositionVS);
        float3 halfAngle = normalize(lightDir + viewDir);

        float blinnTerm = saturate(dot(normal, halfAngle));
        blinnTerm = cosAngle != 0.0 ? blinnTerm : 0.0;
        blinnTerm = pow(blinnTerm, shininess);

        finalColor = (cosAngle + blinnTerm + ambientIntensity) * diffuseColor + float3(0.1f.xxx);
    }
    else
    {
        finalColor = diffuseColor;
    }
    //return float4(0.5f, 0.3f, 0.1f, 1);

    return float4(finalColor,1.0f);
}