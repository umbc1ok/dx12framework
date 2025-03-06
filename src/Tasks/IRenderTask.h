#pragma once

class DepthStencil;
class PipelineState;
class RenderTarget;


class IRenderTask
{
public:
    IRenderTask();
    virtual ~IRenderTask();

    virtual void render() = 0;
    virtual void prepare() = 0;

    PipelineState* m_pipelineState = nullptr;
protected:
};

