// Toy Engine @ 2024
// Author: Hubert Olejnik

#include "shared/shared_cb.h"


struct Vertex
{
    float3 Position;
    float padding;
    float3 Normal;
    float padding2;
    float2 UV;
    float pad;
    float pad2;
};

struct VertexOut
{
    float4 PositionHS   : SV_Position;
    float3 PositionVS   : POSITION0;
    float3 Normal       : NORMAL0;
    uint   MeshletIndex : COLOR0;
    uint   TriangleIndex : COLOR1;
};

struct Meshlet
{
    uint VertOffset;
    uint PrimOffset;
    uint VertCount;
    uint PrimCount;
};


struct Payload
{
    uint MeshletIndices[32];
};

ConstantBuffer<SceneConstantBuffer> InstanceData       : register(b0);
ConstantBuffer<MeshInfo>  MeshInfo                : register(b1);


StructuredBuffer<Vertex>  Vertices                : register(t0);
StructuredBuffer<Meshlet> Meshlets                : register(t1);
StructuredBuffer<uint>    IndexBuffer      : register(t2);
StructuredBuffer<uint>    LocalIndexBuffer  : register(t3);



uint3 GetPrimitive(Meshlet m, uint index)
{
    uint3 primitive;
    uint packedPrimitive = LocalIndexBuffer[m.PrimOffset + index];
    primitive.x = (packedPrimitive >> 0) & 0xFF;
    primitive.y = (packedPrimitive >> 8) & 0xFF;
    primitive.z = (packedPrimitive >> 16) & 0xFF;

    return primitive.xyz;
}

VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    Vertex v = Vertices[IndexBuffer[vertexIndex]];
    
    VertexOut vout;
    vout.PositionVS = mul(float4(v.Position, 1), InstanceData.WorldView).xyz;
    vout.PositionHS = mul(float4(v.Position, 1), InstanceData.WorldViewProj);
    vout.Normal = mul(float4(v.Normal, 0), InstanceData.World).xyz;
    vout.MeshletIndex = meshletIndex;
    vout.TriangleIndex = vertexIndex / 3;
    return vout;
}

[OutputTopology("triangle")]
[NumThreads(128, 1, 1)]
void ms_main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload Payload payload,
    out indices uint3 tris[128],
    out vertices VertexOut verts[64]
)
{
#ifdef CULLING
    uint meshletIndex = MeshInfo.MeshletOffset + payload.MeshletIndices[gid];
#else
    uint meshletIndex = MeshInfo.MeshletOffset + gid; // payload.MeshletIndices[gid];
#endif
    if (meshletIndex >= MeshInfo.MeshletOffset + MeshInfo.MeshletCount)
        return;

    Meshlet m = Meshlets[meshletIndex];

    // Call SetMeshOutputCounts early on in the mesh shader.
    // This call will reserve output memory for storing vertex and primitive attributes (other than the vertex position). 
    // Requesting memory early helps hiding the latency required for memory allocation in the geometry engine.
    // via: https://gpuopen.com/learn/mesh_shaders/mesh_shaders-optimization_and_best_practices/
    SetMeshOutputCounts(m.VertCount, m.PrimCount);


    if(gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid);
    }

    if (gtid < m.VertCount)
    {
        uint vertexIndex = m.VertOffset + gtid;
        verts[gtid] = GetVertexAttributes(meshletIndex, vertexIndex);
    }
}
