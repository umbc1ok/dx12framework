#pragma once
#include "imgui.h"
class Editor
{
public:
    static void create();
    static Editor* get_instance() { return m_instance; }
    Editor();
    ~Editor() {};
    void update();
    void cleanup();
private:
    static Editor* m_instance;
};
