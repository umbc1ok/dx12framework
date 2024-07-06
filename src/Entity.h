#pragma once
#include <string>
#include <vector>

#include "Transform.h"
#include "Component.h"
#include "MainScene.h"

class Component;
class Transform;
class MainScene;


class Entity
{
public:
    Entity(std::string const& name = "Entity");
    static Entity* create(std::string const& name = "Entity");
    static Entity* create(std::string const& guid, std::string const& name = "Entity");

    void destroy_immediate();

    template<class T>
    T* add_component()
    {
        auto component = new T();
        components.emplace_back(component);
        component->entity = this;

        MainScene::get_instance()->add_component_to_start(component);

        // Initialization for internal components
        component->initialize();

        // TODO: Assumption that this entity belongs to the main scene
        // NOTE: Currently we treat manually assigning references in the Game initialization code as if someone would ex. drag a reference
        //       to an object in the Unity's inspector. This means that the Awake call of the constructed components should be called
        //       after all of these references were assigned, or more precisely, when the game has started. We do this manually, by
        //       gathering all the components in the components_to_awake vector and calling Awake on those when the game starts.
        //       If the component is constructed during the gameplay, we call the Awake immediately here.


        if (MainScene::get_instance()->is_running)
        {
            if (!m_is_being_deserialized)
            {
                component->awake();
                component->has_been_awaken = true;

                if (component->enabled())
                {
                    component->on_enabled();
                }
            }
        }
        else
        {
            MainScene::get_instance()->add_component_to_awake(component);
        }


        return component;
    }

    template<class T>
    T* add_component(T* component)
    {
        components.emplace_back(component);
        component->entity = this;

        MainScene::get_instance()->add_component_to_start(component);

        // Initialization for internal components
        component->initialize();

        // TODO: Assumption that this entity belongs to the main scene
        // NOTE: Currently we treat manually assigning references in the Game initialization code as if someone would ex. drag a reference
        //       to an object in the Unity's inspector. This means that the Awake call of the constructed components should be called
        //       after all of these references were assigned, or more precisely, when the game has started. We do this manually, by
        //       gathering all the components in the components_to_awake vector and calling Awake on those when the game starts.
        //       If the component is constructed during the gameplay, we call the Awake immediately here.

        if (MainScene::get_instance()->is_running)
        {
            if (!m_is_being_deserialized)
            {
                component->awake();
                component->has_been_awaken = true;

                if (component->enabled())
                {
                    component->on_enabled();
                }
            }
        }
        else
        {
            MainScene::get_instance()->add_component_to_awake(component);
        }

        return component;
    }

    template<class T, typename... TArgs>
    T* add_component(TArgs&&... args)
    {
        auto component = new T(std::forward<TArgs>(args)...);
        components.emplace_back(component);
        component->entity = this;

        MainScene::get_instance()->add_component_to_start(component);

        // Initialization for internal components
        component->initialize();

        // TODO: Assumption that this entity belongs to the main scene
        // NOTE: Currently we treat manually assigning references in the Game initialization code as if someone would ex. drag a reference
        //       to an object in the Unity's inspector. This means that the Awake call of the constructed components should be called
        //       after all of these references were assigned, or more precisely, when the game has started. We do this manually, by
        //       gathering all the components in the components_to_awake vector and calling Awake on those when the game starts.
        //       If the component is constructed during the gameplay, we call the Awake immediately here.

        if (MainScene::get_instance()->is_running)
        {
            if (!m_is_being_deserialized)
            {
                component->awake();
                component->has_been_awaken = true;

                if (component->enabled())
                {
                    component->on_enabled();
                }
            }
        }
        else
        {
            MainScene::get_instance()->add_component_to_awake(component);
        }


        return component;
    }

    // Component that is not tied to any scene.
    // It will not be awaken, started or updated. Only initialized.
    template<class T>
    T* add_component_internal(T* component)
    {
        components.emplace_back(component);
        component->entity = this;

        // Initialization for internal components
        component->initialize();

        return component;
    }

    template<typename T>
    T* get_component()
    {
        for (auto const& component : components)
        {
            auto comp = component;
            if (comp != nullptr)
                return comp;
        }

        return nullptr;
    }

    template<typename T>
    std::vector<T*> get_components()
    {
        std::vector<T*> vector = {};
        for (auto const& component : components)
        {
            auto comp = component;
            if (comp != nullptr)
                vector.emplace_back(comp);
        }

        return vector;
    }

    std::vector<Component*> components = {};
    std::string name;
    std::string guid;
    size_t hashed_guid;
    Transform* transform = nullptr;

private:
    bool m_is_being_deserialized = false;
};

