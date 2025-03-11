#include "ClearSurfaceTask.h"

#include "DX12Resource/DepthStencil.h"
#include "DX12Resource/RenderTarget.h"

void ClearSurfaceTask::render()
{
    if (m_renderTarget)
    {
        m_renderTarget->setResourceStateToRenderTarget();
        m_renderTarget->clear();
    }
    if (m_depthStencil)
    {
        m_depthStencil->clear();
    }
}

void ClearSurfaceTask::prepare()
{
}
