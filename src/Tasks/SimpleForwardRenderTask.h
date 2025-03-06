#pragma once
#include "Tasks/IRenderTask.h"

class SimpleForwardRenderTask :
    public IRenderTask
{
public:
    SimpleForwardRenderTask();
    ~SimpleForwardRenderTask() override;

    void render() override;
    void prepare() override;

    RenderTarget* m_renderTarget = nullptr;
    DepthStencil* m_depthStencil = nullptr;

};

