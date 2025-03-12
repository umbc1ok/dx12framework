#pragma once
#include "IRenderTask.h"
class FXAATask :
    public IRenderTask
{
public:
    void render() override;
    void prepare() override;

    RenderTarget* m_renderTarget = nullptr;
    RenderTarget* m_previousPassResult = nullptr;
};

