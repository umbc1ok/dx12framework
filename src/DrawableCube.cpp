#include "DrawableCube.h"

#include <iostream>

#include "Window.h"
#include <DirectXMath.h>
#include <imgui.h>

#include "Renderer.h"

using namespace DirectX;
DrawableCube::DrawableCube()
{
    m_vertex_buffer = new VertexBuffer(g_Vertices, sizeof(g_Vertices) / sizeof(Vertex));
    m_index_buffer = new IndexBuffer(g_Indicies, sizeof(g_Indicies) / sizeof(u16));

    hlsl::float3 position(0.0f, 0.0f, 0.0f);
    hlsl::ComposeMatrix(position, hlsl::float4(1.0f, 1.0f, 1.0f, 1.0f), hlsl::float3(1.0f, 1.0f, 1.0f));
}

DrawableCube::~DrawableCube()
{
	delete m_vertex_buffer;
	delete m_index_buffer;
}

void DrawableCube::draw()
{
    auto commandList = Renderer::get_instance()->g_pd3dCommandList;
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, m_vertex_buffer->get_view());
    commandList->IASetIndexBuffer(m_index_buffer->get_view());


    const DirectX::XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    XMMATRIX view = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
    XMMATRIX projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
    XMMATRIX world = XMMatrixTranslation(1.0f, 0.0f, 0.0f);
    world *= XMMatrixRotationX(ImGui::GetTime());
    world *= XMMatrixRotationZ(ImGui::GetTime());
    world *= XMMatrixRotationY(ImGui::GetTime());


    XMMATRIX mvpMatrix = XMMatrixMultiply(world, view);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, projection);

    commandList->SetGraphicsRoot32BitConstants(0, 16, &mvpMatrix, 0);
    commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);
}
