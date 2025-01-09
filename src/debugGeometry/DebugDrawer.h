#pragma once
#include "PipelineState.h"
#include "DX12Wrappers/IndexBuffer.h"
#include "DX12Wrappers/VertexBuffer.h"
#include "../res/shaders/shared/shared_debug_cb.h"

template <typename T>
class ConstantBuffer;


struct DebugDrawing
{
    VertexBuffer* vb;
    IndexBuffer* ib;

    DebugDrawing() {};
};


class DebugDrawer
{
public:
    DebugDrawer();
    ~DebugDrawer();

    void createCamera();

    void draw();
    void drawEditor();

private:
    void drawCameraFrustum();
    void drawMeshletBoundingSpheres();




    std::vector<DebugDrawing*> m_debugDrawings;

    PipelineState* m_pipelineState;

    ConstantBuffer<TransformationMatrices>* m_transformationMatrices;

    TransformationMatrices* matrices;

    bool pauseCamera = false;
};


