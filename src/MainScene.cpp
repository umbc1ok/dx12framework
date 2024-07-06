#include "MainScene.h"

void MainScene::set_instance(Scene* const& scene)
{
    m_instance = scene;
}

Scene* MainScene::get_instance()
{
    return m_instance;
}
