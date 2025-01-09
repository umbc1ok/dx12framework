#include "DebugDrawer.h"

#include <imgui.h>

#include "Camera.h"
#include "Renderer.h"
#include "VisualiserGeometry.h"
#include "DX12Wrappers/ConstantBuffer.h"


DebugDrawer::DebugDrawer()
{
    m_pipelineState = new PipelineState(L"debug/VS_DEBUG.hlsl", L"debug/PS_DEBUG.hlsl", TRADITIONAL);
    m_pipelineState->setWireframe(true);

    m_transformationMatrices = new ConstantBuffer<TransformationMatrices>();
    matrices = new TransformationMatrices();
}
    
DebugDrawer::~DebugDrawer()
{
    delete m_pipelineState;
}

void DebugDrawer::createCamera()
{
    float fov = Camera::getMainCamera()->getFov() * hlsl::DEG2RAD;
    float nearPlane = Camera::getMainCamera()->getNearPlane();
    float farPlane = Camera::getMainCamera()->getFarPlane();
    float aspectRatio = Camera::getMainCamera()->getAspectRatio();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    visualisers::generateFrustumGeometry(fov, nearPlane, farPlane, aspectRatio, vertices, indices);


    DebugDrawing* drawing = new DebugDrawing();
    drawing->vb = new VertexBuffer(vertices.data(), vertices.size());
    drawing->ib = new IndexBuffer(indices.data(), indices.size());

    m_debugDrawings.push_back(drawing);

}

void DebugDrawer::draw()
{

    static bool test = false;
    if (!pauseCamera)
    {
        matrices->World = Camera::getMainCamera()->entity->transform->get_model_matrix();
    }
    matrices->WorldView = hlsl::transpose(Camera::getMainCamera()->getViewMatrix());
    matrices->WorldViewProj = hlsl::transpose(Camera::getMainCamera()->getProjectionMatrix() * (pauseCamera ? Camera::getMainCamera()->getViewMatrix() * matrices->World : hlsl::float4x4()));

    m_transformationMatrices->uploadData(*matrices);


    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    cmd_list->SetPipelineState(m_pipelineState->PSO());
    cmd_list->SetGraphicsRootSignature(m_pipelineState->dx12RootSignature());
    cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_transformationMatrices->setConstantBuffer(0);

    for (uint32_t i = 0; i < m_debugDrawings.size(); i++)
    {
        cmd_list->IASetVertexBuffers(0, 1, m_debugDrawings[i]->vb->get_view());
        cmd_list->IASetIndexBuffer(m_debugDrawings[i]->ib->get_view());
        cmd_list->DrawIndexedInstanced(m_debugDrawings[i]->ib->size(), 1, 0, 0, 0);
    }

}

void DebugDrawer::drawCameraFrustum()
{
}

void DebugDrawer::drawMeshletBoundingSpheres()
{
}

void DebugDrawer::drawEditor()
{
    if(ImGui::Checkbox("Draw Camera Frustum", &pauseCamera))
    {
        Camera::getMainCamera()->freeze(pauseCamera);
    }
}
