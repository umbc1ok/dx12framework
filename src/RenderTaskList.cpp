#include "RenderTaskList.h"

#include "Renderer.h"
#include "DX12Resource/RenderResourcesManager.h"
#include "Tasks/ClearSurfaceTask.h"
#include "Tasks/FullscreenPassTask.h"
#include "Tasks/SimpleForwardRenderTask.h"
#include "Tasks/IRenderTask.h"

void RenderTaskList::prepareMainList()
{
    m_renderTasks.clear();

    // Use RenderTargets as SRVs
    {
        auto* task = new ClearSurfaceTask();
        task->m_renderTarget = Renderer::get_instance()->get_render_resources_manager()->m_forwardRenderTarget;
        m_renderTasks.push_back(task);
    }

    {
        auto* task = new ClearSurfaceTask();
        task->m_renderTarget = Renderer::get_instance()->get_render_resources_manager()->getMainRenderTarget();
        task->m_depthStencil = Renderer::get_instance()->get_render_resources_manager()->getMainDepthStencil();
        m_renderTasks.push_back(task);
    }

    {
        auto* task = new SimpleForwardRenderTask();
        task->m_pipelineState = new PipelineState(L"MS_STANDARD.hlsl", L"PS_BASIC.hlsl", MESH);
        task->m_renderTarget = Renderer::get_instance()->get_render_resources_manager()->m_forwardRenderTarget;
        task->m_depthStencil = Renderer::get_instance()->get_render_resources_manager()->getMainDepthStencil();
        m_renderTasks.push_back(task);
    }


    {
        auto* task = new FullscreenPassTask();
        task->m_pipelineState = new PipelineState(L"ps_fullscreen_test.hlsl", L"ps_fullscreen_test.hlsl", TRADITIONAL);
        task->m_renderTarget = Renderer::get_instance()->get_render_resources_manager()->getMainRenderTarget();
        task->m_previousPassResult = Renderer::get_instance()->get_render_resources_manager()->m_forwardRenderTarget;
        m_renderTasks.push_back(task);
    }


}

void RenderTaskList::renderMainList()
{
    for (auto task : m_renderTasks)
    {
        task->render();
    }
}
