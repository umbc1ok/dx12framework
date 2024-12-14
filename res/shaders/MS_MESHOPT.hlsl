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


ConstantBuffer<SceneConstantBuffer> Globals       : register(b0);
ConstantBuffer<MeshInfo>  MeshInfo                : register(b1);

StructuredBuffer<Vertex>  Vertices                : register(t0);
StructuredBuffer<Meshlet> Meshlets                : register(t1);
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
    int test = m.PrimOffset / 3;
    int val = 0;
    // It seems like meshoptimizer likes to reuse indices from previous primitives
    // so if for example one triangle is ABC, second one is CDE
    // it will not make a duplicate index, but will jest set the primitive offset to the place where C is in the first triangle
    // lot of branching here, need to sort this out.
    if(test * 3 == m.PrimOffset)
    {
        uint packedPrimitive = MeshletTriangleIndices[(m.PrimOffset + val) / 3 + index];
        primitive.x = (packedPrimitive >> 0) & 0xFF;
        primitive.y = (packedPrimitive >> 8) & 0xFF;
        primitive.z = (packedPrimitive >> 16) & 0xFF;
    }
    else if(test * 3 + 1 == m.PrimOffset)
    {
        uint packedPrimitive1 = MeshletTriangleIndices[m.PrimOffset / 3 + index];
        uint packedPrimitive2 = MeshletTriangleIndices[m.PrimOffset / 3 + index + 1];
        primitive.x = (packedPrimitive1 >> 8) & 0xFF;
        primitive.y = (packedPrimitive1 >> 16) & 0xFF;
        primitive.z = (packedPrimitive2 >> 0) & 0xFF;
    }
    else
    {
        uint packedPrimitive1 = MeshletTriangleIndices[m.PrimOffset / 3 + index];
        uint packedPrimitive2 = MeshletTriangleIndices[m.PrimOffset / 3 + index + 1];
        primitive.x = (packedPrimitive1 >> 16) & 0xFF;
        primitive.y = (packedPrimitive2 >> 0) & 0xFF;
        primitive.z = (packedPrimitive2 >> 8) & 0xFF;
    }
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
