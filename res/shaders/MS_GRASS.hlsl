#include "shared/shared_cb.h"

ConstantBuffer<SceneConstantBuffer> Globals   : register(b0);
ConstantBuffer<Wind> wind_data                : register(b1);

RWStructuredBuffer<Blade>  blades               : register(u0);



#define ROOT_SIG "CBV(b0), \
                  CBV(b1), \
                  UAV(u0) \
                  "



struct VertexOut
{
    float4 PositionHS   : SV_Position;
    float3 PositionVS   : POSITION0;
    float3 Normal       : NORMAL0;
    float   stiffness : COLOR0;
};

VertexOut TransformVertex(float3 position, float stiffness)
{

    VertexOut vout;
    vout.PositionVS = mul(float4(position, 1), Globals.WorldView).xyz;
    vout.PositionHS = mul(float4(position, 1), Globals.WorldViewProj);
    vout.Normal = mul(float4(position, 0), Globals.World).xyz;
    vout.stiffness = stiffness;
    return vout;
}

[RootSignature(ROOT_SIG)]
[OutputTopology("triangle")]
[NumThreads(1, 1, 1)]
void ms_main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out vertices VertexOut verts[11],
    out indices uint3 tris[12]
)
{
    // 1/60s
    float FRAME_TIME  = 0.01666;
    // 1 / 1000 kg
    float GRASS_POINT_MASS = 0.001;
    // This is just a drag coefficient
    float DRAG_COEFFICIENT = 0.0004;
    // Top position is the point where the tip should be if it wasnt for wind
    float3 top_position = blades[gid].m_root_position + float3(0.0f, blades[gid].m_height, 0.0f);
    float3 middle_position = blades[gid].m_root_position + float3(0.0f, blades[gid].m_height / 2.0f, 0.0f);

    // This is not real resistance, it's a simplification of it
    float3 wind_resistance_force = float3(wind_data.force * wind_data.direction.x * DRAG_COEFFICIENT, 0.0f, wind_data.force * wind_data.direction.y * DRAG_COEFFICIENT);


    float3 tip_restoration_force = 0.0f;
    float3 mid_restoration_force = 0.0f;
    // Calculate restoration force
    {
        if(length(blades[gid].m_tip_position - top_position) != 0.0f)
        {
            tip_restoration_force =  blades[gid].m_tip_stiffness * (top_position - blades[gid].m_tip_position) * wind_data.restoration_strength;
        }
        if(length(blades[gid].m_middle_position - middle_position) != 0.0f)
        {
            mid_restoration_force =  blades[gid].m_tip_stiffness * (middle_position - blades[gid].m_middle_position) * wind_data.restoration_strength * 2.5f;
        }
    }
    
    // Calculate force and accelaration
    float3 tip_force = wind_resistance_force + tip_restoration_force;
    float3 mid_force = wind_resistance_force + mid_restoration_force;
    float3 tip_acceleration = tip_force.xyz / GRASS_POINT_MASS;
    float3 mid_acceleration = mid_force.xyz / GRASS_POINT_MASS;

    // Euler method
    {
        blades[gid].m_middle_position = blades[gid].m_middle_position + blades[gid].movement_speed_middle * FRAME_TIME;
        blades[gid].m_tip_position = blades[gid].m_tip_position + blades[gid].movement_speed_tip * FRAME_TIME;
        blades[gid].movement_speed_middle = blades[gid].movement_speed_middle + FRAME_TIME * mid_acceleration;
        blades[gid].movement_speed_tip = blades[gid].movement_speed_tip + FRAME_TIME * tip_acceleration;
    }

    // Damp so the force doesn't rise infinetely
    {
        float damping = 0.85f;
        blades[gid].movement_speed_middle *= damping;
        blades[gid].movement_speed_tip *= damping;
    }
    

    // Restraints
    {
        float3 root_to_mid = blades[gid].m_root_position - blades[gid].m_middle_position;
        float current_distance = length(root_to_mid);
        float target_distance = blades[gid].m_height / 2.0f;


        if (current_distance != target_distance)
        {
            float3 direction = normalize(root_to_mid);
            blades[gid].m_middle_position += direction * (current_distance - target_distance);
        }

        float3 mid_to_tip = blades[gid].m_middle_position - blades[gid].m_tip_position;
        float current_distance_mid_to_tip = length(mid_to_tip);

        if (current_distance_mid_to_tip != target_distance)
        {
            float3 direction = normalize(mid_to_tip);
            blades[gid].m_tip_position += direction * (current_distance_mid_to_tip - target_distance);
        }
    }

    // Output vertices
    SetMeshOutputCounts(11, 12);

    Blade b = blades[gid];
    verts[0] = TransformVertex(b.m_root_position, b.m_tip_stiffness);
    verts[1] = TransformVertex(b.m_middle_position,b.m_tip_stiffness);
    verts[2] = TransformVertex(b.m_tip_position, b.m_tip_stiffness);
    verts[3] = TransformVertex(b.m_root_position + float3(0.01f, 0, 0.0f),b.m_tip_stiffness);
    verts[4] = TransformVertex(b.m_root_position + float3(-0.01f, 0, 0.0f),b.m_tip_stiffness);
    verts[5] = TransformVertex(b.m_middle_position + float3(0.01f, 0, 0.0f),b.m_tip_stiffness);
    verts[6] = TransformVertex(b.m_middle_position + float3(-0.01f, 0, 0.0f),b.m_tip_stiffness);

    verts[7] = TransformVertex(b.m_root_position + float3(0, 0, 0.01f),b.m_tip_stiffness);
    verts[8] = TransformVertex(b.m_root_position + float3(0, 0, -0.01f),b.m_tip_stiffness);

    verts[9] = TransformVertex(b.m_middle_position + float3(0, 0, 0.01f),b.m_tip_stiffness);
    verts[10] = TransformVertex(b.m_middle_position + float3(0, 0, -0.01f),b.m_tip_stiffness);

    tris[0] = uint3(0,3,5);
    tris[1] = uint3(5,0,1);
    tris[2] = uint3(1,6,0);
    tris[3] = uint3(0,4,6);
    tris[4] = uint3(2,5,1);
    tris[5] = uint3(1,6,2);

    tris[6] = uint3(7,0,9);
    tris[7] = uint3(0,10,8);
    tris[8] = uint3(10,0,1);
    tris[9] = uint3(0,9,1);
    tris[10] = uint3(9,2,1);
    tris[11] = uint3(10,1,2);
    
    ////
} 