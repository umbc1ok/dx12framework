#include "Game.h"

#include "Entity.h"
#include "Model.h"

void Game::init()
{
    auto entity = Entity::create("Bunny");
    auto model = Model::create("./res/models/nobby/nobby.obj");
    entity->add_component(model);
    entity->transform->set_position(hlsl::float3(15.0f, 0.0f, 0.0f));

    auto entity1 = Entity::create("Nanosuit");
    auto model1 = Model::create("./res/models/buddha/buddha3.obj");
    entity1->add_component(model1);
    entity1->transform->set_scale(hlsl::float3(10.0f, 10.0f, 10.0f));

}

