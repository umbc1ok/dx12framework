#pragma once
#ifdef __cplusplus
using float4x4 = hlsl::float4x4;
using float3 = hlsl::float3;
using float2 = hlsl::float2;
using float4 = hlsl::float4;
using uint = unsigned int;
#endif



#ifdef __cplusplus
__declspec(align(256))
#endif
struct TransformationMatrices
{
    float4x4 World;
    float4x4 WorldView;
    float4x4 WorldViewProj;
};