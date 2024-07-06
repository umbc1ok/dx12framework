#include "Component.h"

#include "Entity.h"

#include <imgui.h>

#include "MainScene.h"
#include "utils/Utils.h"

Component::Component()
{
    guid = olej_utils::generate_guid();
}

void Component::initialize()
{
}

void Component::uninitialize()
{
}

void Component::reprepare()
{
}

void Component::awake()
{
}

void Component::start()
{
}

void Component::update()
{
}

void Component::on_enabled()
{
}

void Component::on_disabled()
{
}

void Component::on_destroyed()
{
}

void Component::destroy_immediate()
{
    assert(entity != nullptr);

    if (!has_been_awaken)
    {
        MainScene::get_instance()->remove_component_to_awake(this);
    }

    if (!has_been_started)
    {
        MainScene::get_instance()->remove_component_to_start(this);
    }

    set_can_tick(false);
    set_enabled(false);
    uninitialize();

    olej_utils::swap_and_erase(entity->components, this);
    entity = nullptr;
}

void Component::draw_editor()
{
    if (ImGui::Button("Remove component", ImVec2(-FLT_MIN, 20.0f)))
    {
        destroy_immediate();
    }
}

void Component::set_can_tick(bool const value)
{
    if (m_can_tick != value)
    {

        if (value)
            MainScene::get_instance()->tickable_components.emplace_back(this);
        else
            olej_utils::swap_and_erase(MainScene::get_instance()->tickable_components, this);

    }

    m_can_tick = value;
}

bool Component::get_can_tick() const
{
    return m_can_tick;
}

void Component::set_enabled(bool const value)
{
    if (value == m_enabled)
        return;

    m_enabled = value;

    if (value)
    {
        on_enabled();
    }
    else
    {
        on_disabled();
    }
}

bool Component::enabled() const
{
    return m_enabled;
}
