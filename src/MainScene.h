#pragma once
#include "Scene.h"

class MainScene final : public Scene
{
public:
    virtual ~MainScene() = default;

    static void set_instance(Scene* const& scene);

    static Scene* get_instance();

    MainScene(MainScene const&) = delete;
    void operator=(MainScene const&) = delete;

private:
    inline static Scene* m_instance;
};