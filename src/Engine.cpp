#include "Engine.h"

#include <iostream>

#include "Window.h"
#include "Editor.h"
#include "Entity.h"
#include "MainScene.h"
#include "Renderer.h"
#include <Camera.h>

#include "Input.h"


void Engine::setup()
{
    Window::create();
    Renderer::create();
    Editor::create();
    Input::create();
    create_game();
    Renderer::get_instance()->camera_entity = Entity::create("Camera");
    Camera::create();
    Renderer::get_instance()->camera_entity->add_component(Camera::get_main_camera());
}

void Engine::run()
{
    bool run = true;
    while (run)
    {
        MainScene::get_instance()->run_frame();
        Input::get_instance()->update();
        Renderer::get_instance()->start_frame();
        Renderer::get_instance()->render();
        Editor::get_instance()->update();
        Renderer::get_instance()->end_frame();
        MSG msg;

        // Poll events after running engine to prevent exceptions when closing the window
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                run = false;
            }
        }
    }
}

void Engine::cleanup()
{
    Editor::get_instance()->cleanup();
    Renderer::get_instance()->cleanup();
}

bool Engine::is_game_running()
{
    return true;
}

void Engine::create_game()
{
    auto const main_scene = new Scene();
    MainScene::set_instance(main_scene);
    auto entity = Entity::create("Entity");
}
