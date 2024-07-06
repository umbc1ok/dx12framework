#include <vector>

#include "Component.h"

class Entity;

class Transform
{
public:
    explicit Transform(Entity* const& t_entity);
    ~Transform();

    void set_parent(Transform* const& new_parent);

    std::vector<Transform*> children;
    Transform* parent = {};
    Entity* entity = {};
private:
    void add_child(Transform* const& transform);
    void remove_child(Transform* const& transform);

    bool m_local_dirty = false;
}
