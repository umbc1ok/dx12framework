#include "DebugDrawer.h"

#include <imgui.h>

#include "Camera.h"
#include "Renderer.h"
#include "VisualiserGeometry.h"
#include "DX12Wrappers/ConstantBuffer.h"
#include "Transform.h"

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
    drawing->transform = Camera::getMainCamera()->entity->transform;
    drawing->type = DebugDrawingType::CAMERA_FRUSTUM;
    m_debugDrawings.push_back(drawing);

}

DebugDrawing* DebugDrawer::registerDebugDrawing(std::vector<uint32_t> indices, std::vector<Vertex> vertices, DebugDrawingType type)
{
    DebugDrawing* drawing = new DebugDrawing();
    drawing->vb = new VertexBuffer(vertices.data(), vertices.size());
    drawing->ib = new IndexBuffer(indices.data(), indices.size());
    drawing->type = type;
    drawing->active = false;
    drawing->transform = new Transform();
    m_debugDrawings.push_back(drawing);

    return drawing;
}

void DebugDrawer::eraseDebugDrawing(DebugDrawing* drawing)
{
    auto it = std::find(m_debugDrawings.begin(), m_debugDrawings.end(), drawing);
    if (it != m_debugDrawings.end())
    {
        m_debugDrawings.erase(it);
    }

    delete drawing;
}

void DebugDrawer::draw()
{



    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    cmd_list->SetPipelineState(m_pipelineState->PSO());
    cmd_list->SetGraphicsRootSignature(m_pipelineState->dx12RootSignature());
    cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (uint32_t i = 0; i < m_debugDrawings.size(); i++)
    {

        if (m_debugDrawings[i]->type == DebugDrawingType::CAMERA_FRUSTUM)
        {
            if (!pauseCamera)
            {
                m_cachedCameraWorld = Camera::getMainCamera()->entity->transform->get_model_matrix();
                continue;
            }
            matrices->World = m_cachedCameraWorld;
            matrices->WorldView = hlsl::transpose(Camera::getMainCamera()->getViewMatrix());
            matrices->WorldViewProj = hlsl::transpose(Camera::getMainCamera()->getProjectionMatrix() * (pauseCamera ? Camera::getMainCamera()->getViewMatrix() * matrices->World : hlsl::float4x4()));
        }
        else
        {
            if (!m_debugDrawings[i]->active)
            {
                continue;
            }
            matrices->World = m_debugDrawings[i]->transform->get_model_matrix();
            matrices->WorldView = hlsl::transpose(Camera::getMainCamera()->getViewMatrix());
            matrices->WorldViewProj = hlsl::transpose(Camera::getMainCamera()->getProjectionMatrix() * Camera::getMainCamera()->getViewMatrix() * matrices->World);
        }



        m_transformationMatrices->uploadData(*matrices);
        m_transformationMatrices->setConstantBuffer(0);

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
