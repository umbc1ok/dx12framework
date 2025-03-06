#include "SimpleForwardRenderTask.h"

#include "MainScene.h"
#include "Renderer.h"
#include "DX12Resource/RenderTarget.h"
#include "IRenderTask.h"


SimpleForwardRenderTask::SimpleForwardRenderTask()
{
}

SimpleForwardRenderTask::~SimpleForwardRenderTask()
{
}

void SimpleForwardRenderTask::render()
{
    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;

    cmd_list->SetPipelineState(m_pipelineState->PSO());
    auto dsv = m_depthStencil->heap()->GetCPUDescriptorHandleForHeapStart();
    cmd_list->OMSetRenderTargets(1, m_renderTarget->getHandle(), FALSE, &dsv);


    MainScene::get_instance()->runFrame();
}

void SimpleForwardRenderTask::prepare()
{
}
