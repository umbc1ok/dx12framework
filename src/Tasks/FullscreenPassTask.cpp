#include "FullscreenPassTask.h"

#include "Renderer.h"
#include "DX12Resource/RenderTarget.h"
void FullscreenPassTask::render()
{
    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;

    cmd_list->SetPipelineState(m_pipelineState->PSO());
    cmd_list->SetGraphicsRootSignature(m_pipelineState->dx12RootSignature());
    cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd_list->OMSetRenderTargets(1, m_renderTarget->getHandle(), FALSE, nullptr);
    m_previousPassResult->bind(m_pipelineState, "mainRT");

    cmd_list->DrawInstanced(6, 1, 0, 0); // Fullscreen triangle
}

void FullscreenPassTask::prepare()
{
}
