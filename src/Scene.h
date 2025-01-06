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

    void addChild(Entity* const& entity);
    void removeChild(Entity* const& entity);

    void addComponentToAwake(Component* const& component);
    void removeComponentFromAwake(Component* const& component);

    void addComponentToStart(Component* const& component);
    void removeComponentFromStart(Component* const& component);

    [[nodiscard]] Entity* getEntityByGUID(std::string const& guid) const;
    [[nodiscard]] Component* getComponentByGuid(std::string const& guid) const;

    void runFrame();

    bool is_running = false;

    std::vector<Entity*> m_entities = {};
    std::vector<Component*> m_tickableComponents = {};

private:
    std::vector<Component*> m_componentsToAwake = {};
    std::vector<Component*> m_componentsToStart = {};
};

