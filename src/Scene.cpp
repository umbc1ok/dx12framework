#include "Scene.h"
#include "Entity.h"
#include <cassert>

#include "utils/Utils.h"

void Scene::unload()
{
    // TODO: We should probably cache top level entities somewhere or maybe assign them to dummy root entity
    // (I don't really like either of these).
    std::vector <Entity*> top_level_entities = {};
    for (auto const& entity : m_entities)
    {
        if (entity->transform->parent == nullptr)
            top_level_entities.emplace_back(entity);
    }

    for (auto const& entity : top_level_entities)
    {
        entity->destroy_immediate();
    }
}

void Scene::addChild(Entity* const& entity)
{
    m_entities.emplace_back(entity);
}

void Scene::removeChild(Entity* const& entity)
{
    auto const it = std::ranges::find(m_entities, entity);

    assert(it != m_entities.end());

    if (it == m_entities.end())
        return;

    m_entities.erase(it);
}

void Scene::addComponentToAwake(Component* const& component)
{
    m_componentsToAwake.emplace_back(component);
}

void Scene::removeComponentFromAwake(Component* const& component)
{
    // We don't want to change the order of the components Awakes
    if (auto const position = std::ranges::find(m_componentsToAwake, component); position != m_componentsToAwake.end())
    {
        m_componentsToAwake.erase(position);
    }
}

void Scene::addComponentToStart(Component* const& component)
{
    m_componentsToStart.emplace_back(component);
}

void Scene::removeComponentFromStart(Component* const& component)
{
    // We don't want to change the order of the components Starts
    if (auto const position = std::ranges::find(m_componentsToStart, component); position != m_componentsToStart.end())
    {
        m_componentsToStart.erase(position);
    }
}
Entity* Scene::getEntityByGUID(std::string const& guid) const
{
    // TODO: Cache entities in an unordered map with guids as keys
    for (auto const& entity : m_entities)
    {
        if (entity->guid == guid)
        {
            return entity;
        }
    }

    return nullptr;
}

Component* Scene::getComponentByGuid(std::string const& guid) const
{
    // TODO: Cache components in an unordered map with guids as keys
    for (auto const& entity : m_entities)
    {
        for (auto const& component : entity->components)
        {
            if (component->guid == guid)
            {
                return component;
            }
        }
    }

    return nullptr;
}

void Scene::runFrame()
{
    // Call Awake on every component that was constructed before running the first frame
    if (!is_running)
    {
        is_running = true;

        auto const copy_components_to_awake = this->m_componentsToAwake;
        for (auto const& component : m_componentsToAwake)
        {
            component->awake();
            component->has_been_awaken = true;

            if (component->enabled())
                component->on_enabled();
        }

        m_componentsToAwake.clear();

        // Release the capacity
        m_componentsToAwake.shrink_to_fit();
    }

    // Call Start on every component that hasn't been started yet
    auto const copy_components_to_start = this->m_componentsToStart;
    for (auto const& component : copy_components_to_start)
    {
        component->start();
        component->has_been_started = true;

        olej_utils::swapAndErase(this->m_componentsToStart, component);
    }

    // Call Update on every tickable component

    // Scene Entities vector might be modified by components, ex. when they create new entities
    // TODO: Destroying entities is not handled properly. But we don't support any way of destroying an entity anyway, so...

    // TODO: Don't make a copy of tickable components every frame, since they will most likely not change frequently, so we might
    //       just manually manage the vector?
    auto const components_copy = m_tickableComponents;
    for (auto const& component : components_copy)
    {
        if (component == nullptr || component->entity == nullptr || !component->enabled())
            continue;

        component->update();
    }
}

