#pragma once
#include "IRenderTask.h"
class ClearSurfaceTask :
    public IRenderTask
{
public:

    void render() override;
    void prepare() override;


    RenderTarget* m_renderTarget = nullptr;
    DepthStencil* m_depthStencil = nullptr;
};

