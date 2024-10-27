#ifdef __cplusplus
using float4x4 = hlsl::float4x4;
using uint = unsigned int;
#endif

#ifdef __cplusplus
__declspec(align(256))
#endif

#define DRAW_NORMAL 0
#define DRAW_MESHLETS 1
#define DRAW_TRIANGLES 2

struct SceneConstantBuffer
{
    float4x4 World;
    float4x4 WorldView;
    float4x4 WorldViewProj;
    uint   DrawFlag;
};