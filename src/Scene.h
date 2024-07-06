#pragma once

#include <vector>

#include "Component.h"

class Entity;

class Scene
{
public:
    virtual ~Scene() = default;
    Scene() = default;
    virtual void unload();

    void add_child(Entity* const& entity);
    void remove_child(Entity* const& entity);

    void add_component_to_awake(Component* const& component);
    void remove_component_to_awake(Component* const& component);

    void add_component_to_start(Component* const& component);
    void remove_component_to_start(Component* const& component);

    [[nodiscard]] Entity* get_entity_by_guid(std::string const& guid) const;
    [[nodiscard]] Component* get_component_by_guid(std::string const& guid) const;

    void run_frame();

    bool is_running = false;

    std::vector<Entity*> entities = {};
    std::vector<Component*> tickable_components = {};

private:
    std::vector<Component*> components_to_awake = {};
    std::vector<Component*> components_to_start = {};
};

