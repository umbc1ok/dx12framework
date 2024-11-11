// Toy Engine @ 2024
// Author: Hubert Olejnik

#include "shared/shared_cb.h"

struct MeshInfo
{
    uint IndexBytes;
    uint MeshletOffset;
};

struct Vertex
{
    float3 Position;
    float3 Normal;
    float2 UV;
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

ConstantBuffer<SceneConstantBuffer> Globals       : register(b0);
ConstantBuffer<MeshInfo>  MeshInfo                : register(b1);

StructuredBuffer<Vertex>  Vertices                : register(t0);
StructuredBuffer<Meshlet> Meshlets                : register(t1);
// has global indices for every meshlet
StructuredBuffer<uint>    MeshletIndexBuffer      : register(t2);

StructuredBuffer<uint>    MeshletTriangleIndices  : register(t3);


#define ROOT_SIG "CBV(b0), \
                  RootConstants(b1, num32bitconstants=2), \
                  SRV(t0), \
                  SRV(t1), \
                  SRV(t2), \
                  SRV(t3)"


uint3 GetPrimitive(Meshlet m, uint index)
{
    uint3 primitive;
    // i have so many fucking questions
    uint packedPrimitive = MeshletTriangleIndices[index];
    primitive.z = (packedPrimitive >> 0) & 0xFF;  // Least significant byte
    primitive.y = (packedPrimitive >> 8) & 0xFF;  // Second byte
    primitive.x = (packedPrimitive >> 16) & 0xFF; // Third byte
    return primitive.xyz;
}

VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    Vertex v = Vertices[MeshletIndexBuffer[vertexIndex]];
    
    VertexOut vout;
    vout.PositionVS = mul(float4(v.Position, 1), Globals.WorldView).xyz;
    vout.PositionHS = mul(float4(v.Position, 1), Globals.WorldViewProj);
    vout.Normal = mul(float4(v.Normal, 0), Globals.World).xyz;
    vout.MeshletIndex = meshletIndex;
    vout.TriangleIndex = vertexIndex / 3;
    return vout;
}

[RootSignature(ROOT_SIG)]
[OutputTopology("triangle")]
[NumThreads(124, 1, 1)]
void ms_main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint3 tris[124],
    out vertices VertexOut verts[64]
)
{
    Meshlet m = Meshlets[gid];
    
    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    if(gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid);
    }

    if (gtid < m.VertCount)
    {
        uint vertexIndex = m.VertOffset + gtid;
        verts[gtid] = GetVertexAttributes(gid, vertexIndex);
    }
}
