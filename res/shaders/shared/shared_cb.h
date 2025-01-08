#pragma once
#ifdef __cplusplus
using float4x4 = hlsl::float4x4;
using float3 = hlsl::float3;
using float2 = hlsl::float2;
using float4 = hlsl::float4;
using uint = unsigned int;
#endif


#define DRAW_NORMAL 0
#define DRAW_MESHLETS 1
#define DRAW_TRIANGLES 2

#ifdef __cplusplus
__declspec(align(256))
#endif
struct SceneConstantBuffer
{
    float4x4 World;
    float4x4 WorldView;
    float4x4 WorldViewProj;
    
    uint   DrawFlag;
    float time;
};


#ifdef __cplusplus
__declspec(align(256))
#endif
struct CameraConstants
{
    float4      Planes[6];
    float3      CullViewPosition;
};


#ifdef __cplusplus
__declspec(align(256))
#endif
struct MeshInfo
{
    uint IndexBytes;
    uint MeshletOffset;
    uint MeshletCount;
};



struct Blade
{
    float3 m_root_position;
    float m_height;
    float3 m_middle_position;
    float m_tip_stiffness;
    float3 m_tip_position;
    float padding;
    float3 movement_speed_middle;
    float padding2;
    float3 movement_speed_tip;
    float padding3;
};

#ifdef __cplusplus
__declspec(align(256))
#endif
struct Wind
{
    // needs to be a normalized vector
    float2 direction;
    float force;
    float restoration_strength;
};

#ifdef __cplusplus
__declspec(align(256))
#endif
struct MeshletCullData
{
    float4 BoundingSphere;
    uint   NormalCone;
    float  ApexOffset;
};