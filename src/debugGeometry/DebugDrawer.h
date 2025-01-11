#pragma once
#include "PipelineState.h"
#include "Transform.h"
#include "DX12Wrappers/IndexBuffer.h"
#include "DX12Wrappers/VertexBuffer.h"
#include "../res/shaders/shared/shared_debug_cb.h"


template <typename T>
class ConstantBuffer;

enum class DebugDrawingType : uint8_t
{
    CAMERA_FRUSTUM,
    BENCHMARK_SPHERE,

    MAX
};

struct DebugDrawing
{
    VertexBuffer* vb;
    IndexBuffer* ib;

    Transform* transform = nullptr;

    DebugDrawingType type;
    bool active = false;

    ConstantBuffer<TransformationMatrices>* m_transformationMatrices;

    TransformationMatrices* matrices;

    DebugDrawing();

};


class DebugDrawer
{
public:
    DebugDrawer();
    ~DebugDrawer();

    void createCamera();

    DebugDrawing* registerDebugDrawing(std::vector<uint32_t> indices, std::vector<Vertex> vertices, DebugDrawingType type);
    void eraseDebugDrawing(DebugDrawing* drawing);

    void draw();
    void drawEditor();

private:
    void drawCameraFrustum();
    void drawMeshletBoundingSpheres();


    std::vector<DebugDrawing*> m_debugDrawings;

    PipelineState* m_pipelineState;



    hlsl::float4x4 m_cachedCameraWorld;

    bool pauseCamera = false;
};


