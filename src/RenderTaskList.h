#pragma once
#include <vector>

class IRenderTask;

class RenderTaskList
{
public:
    RenderTaskList() = default;
    ~RenderTaskList() = default;

    void prepareMainList();
    void renderMainList();
private:
    std::vector<IRenderTask*> m_renderTasks;
};

