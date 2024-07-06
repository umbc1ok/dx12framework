#include "Engine.h"

#include "Window.h"
#include "Editor.h"
#include "Entity.h"
#include "MainScene.h"


void Engine::setup()
{
    Window::create();
    Editor::create();
    create_game();
}

void Engine::run()
{
    bool run = true;
    while (run)
    {
        Window::get_instance()->start_frame();
        Editor::get_instance()->update();
        Window::get_instance()->end_frame();
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
    Window::get_instance()->cleanup();
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
