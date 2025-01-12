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

#define ROOT_SIG "CBV(b0), \
                  CBV(b1), \
                  CBV(b2), \
                  SRV(t0), \
                  SRV(t1), \
                  SRV(t2), \
                  SRV(t3), \
                  SRV(t4) \
                  "





#define THREADS_PER_WAVE 32 // Assumes availability of wave size of 32 threads

// Pre-defined threadgroup sizes for AS & MS stages
#define AS_GROUP_SIZE THREADS_PER_WAVE


struct Payload
{
    uint MeshletIndices[AS_GROUP_SIZE];
};

bool IsConeDegenerate(MeshletCullData c)
{
    return (c.NormalCone >> 24) == 0xff;
}

float4 UnpackCone(uint packed)
{
    float4 v;
    v.x = float((packed >> 0) & 0xFF);
    v.y = float((packed >> 8) & 0xFF);
    v.z = float((packed >> 16) & 0xFF);
    v.w = float((packed >> 24) & 0xFF);

    v = v / 255.0;
    v.xyz = v.xyz * 2.0 - 1.0;

    return v;
}


ConstantBuffer<SceneConstantBuffer>     InstanceData            : register(b0);
ConstantBuffer<MeshInfo>                MeshInfo                : register(b1);
ConstantBuffer<CameraConstants>         CameraData              : register(b2);

// StructuredBuffer<Vertex>  Vertices                : register(t0);
// StructuredBuffer<Meshlet> Meshlets                : register(t1);
// StructuredBuffer<uint>    MeshletIndexBuffer      : register(t2);
// StructuredBuffer<uint>    MeshletTriangleIndices  : register(t3);

StructuredBuffer<MeshletCullData>              meshletcullData         : register(t4);

// The groupshared payload data to export to dispatched mesh shader threadgroups
groupshared Payload s_Payload;

bool IsVisible(MeshletCullData c, float4x4 world, float scale, float3 viewPos)
{
    // if ((Instance.Flags & CULL_FLAG) == 0)
    //     return true;

    // Do a cull test of the bounding sphere against the view frustum planes.
    float4 center = mul(float4(c.BoundingSphere.xyz, 1), world);
    float radius = c.BoundingSphere.w * scale;

    for (int i = 0; i < 6; ++i)
    {
        if (dot(center, CameraData.Planes[i]) < -radius)
        {
            return false;
        }
    }

    // Do normal cone culling
    if (IsConeDegenerate(c))
        return true; // Cone is degenerate - spread is wider than a hemisphere.

    // Unpack the normal cone from its 8-bit uint compression
    float4 normalCone = UnpackCone(c.NormalCone);

    // Transform axis to world space
    float3 axis = normalize(mul(float4(normalCone.xyz, 0), world)).xyz;

    // Offset the normal cone axis from the meshlet center-point - make sure to account for world scaling
    float3 apex = center.xyz - axis * c.ApexOffset * scale;
    float3 view = normalize(viewPos - apex);

    // The normal cone w-component stores -cos(angle + 90 deg)
    // This is the min dot product along the inverted axis from which all the meshlet's triangles are backface
    if (dot(view, -axis) > normalCone.w)
    {
        return false;
    }

    // All tests passed - it will merit pixels
    return true;
}


[RootSignature(ROOT_SIG)]
[NumThreads(AS_GROUP_SIZE, 1, 1)]
void as_main(uint gtid : SV_GroupThreadID, uint dtid : SV_DispatchThreadID, uint gid : SV_GroupID)
{
    bool visible = false;

    // Check bounds of meshlet cull data resource
    if (MeshInfo.MeshletOffset + dtid < MeshInfo.MeshletCount)
    {
        // Do visibility testing for this thread
        float scale = 1.0f;
        visible = true;//IsVisible(meshletcullData[MeshInfo.MeshletOffset + dtid], InstanceData.World, scale, CameraData.CullViewPosition);
    }
    // Compact visible meshlets into the export payload array
    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        s_Payload.MeshletIndices[index] = dtid;
    }

    // Dispatch the required number of MS threadgroups to render the visible meshlets
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, s_Payload);
}