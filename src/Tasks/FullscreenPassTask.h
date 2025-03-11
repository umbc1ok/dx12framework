#pragma once
#include "IRenderTask.h"
class FullscreenPassTask :
    public IRenderTask
{
public:
    void render() override;
    void prepare() override;


    RenderTarget* m_renderTarget = nullptr;
    RenderTarget* m_previousPassResult = nullptr;
private:

};

