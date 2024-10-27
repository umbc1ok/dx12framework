#include "Game.h"

#include "Entity.h"
#include "Model.h"

void Game::init()
{
    auto entity = Entity::create("Bunny");
    auto model = Model::create("./res/models/bunny/bunny.obj");
    entity->add_component(model);
    entity->transform->set_position(hlsl::float3(0.0f, 0.0f, 0.0f));

}

