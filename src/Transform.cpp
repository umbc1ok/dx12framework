#include "Transform.h"
#include <ranges>
#include <cassert>

Transform::Transform(Entity* const& t_entity)
    : entity(t_entity)
{
}


void Transform::set_parent(Transform* const& new_parent)
{
    if (new_parent == nullptr)
    {
        if (parent == nullptr)
            return;

        parent->remove_child(this);
        m_local_dirty = true;
        return;
    }

    if (parent != nullptr)
    {
        parent->remove_child(this);
    }

    new_parent->add_child(this));
    m_local_dirty = true;
}

void Transform::add_child(Transform* const& transform)
{
    children.emplace_back(transform);
    transform->parent = this;
}

void Transform::remove_child(Transform* const& transform)
{
    assert(transform->parent == this);

    auto const it = std::ranges::find(children, transform);

    if (it == children.end())
        return;

    children.erase(it);

    transform->parent = nullptr;
}
