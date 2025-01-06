#include "Entity.h"

#include "Engine.h"
#include "utils/maths.h"
#include "utils/Utils.h"


Entity::Entity(std::string const& t_name)
    : name(t_name)
{

}


Entity* Entity::create(std::string const& name)
{
    auto entity = new Entity(name);
    entity->guid = olej_utils::generateGUID();
    std::hash<std::string> constexpr hasher;
    entity->hashed_guid = hasher(entity->guid);
    entity->transform = new Transform(entity);
    MainScene::get_instance()->addChild(entity);
    return entity;
}

Entity* Entity::create(std::string const& guid, std::string const& name)
{
    auto entity = new Entity(name);
    entity->guid = guid;
    std::hash<std::string> constexpr hasher;
    entity->hashed_guid = hasher(entity->guid);
    entity->transform = new Transform(entity);
    MainScene::get_instance()->addChild(entity);
    return entity;
}

void Entity::destroy_immediate()
{
    for (uint32_t i = 0; i < components.size(); ++i)
    {
        components[i]->on_destroyed();
    }

    // NOTE: We need to keep a pointer to this object to keep it alive for the duration of this function.
    auto const ptr = this;
    MainScene::get_instance()->removeChild(ptr);

    for (unsigned int i = 0; i < components.size(); ++i)
    {
        if (!components[i]->has_been_awaken)
        {
            MainScene::get_instance()->removeComponentFromAwake(components[i]);
        }

        if (!components[i]->has_been_started)
        {
            MainScene::get_instance()->removeComponentFromStart(components[i]);
        }

        components[i]->set_can_tick(false);

        if (Engine::is_game_running())
            components[i]->set_enabled(false);

        components[i]->uninitialize();
        components[i]->entity = nullptr;
    }

    auto const children_copy = transform->children;
    for (auto const& child : children_copy)
    {
        child->entity->destroy_immediate();
    }

    if (!transform->parent)
    {
        transform->set_parent(nullptr);
    }
    delete this;
}
