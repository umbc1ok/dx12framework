#include "Game.h"

#include "Entity.h"
#include "Model.h"
#include "Grass.h"
void Game::init()
{
    //auto entity1 = Entity::create("Model");
    ////auto model = Model::create("./res/models/plane.obj");
    //auto model = Model::create("./res/models/bunny/bunny.obj");
    //entity1->add_component(model);

    //auto entity2 = Entity::create("Nanosuit");
    ////auto model = Model::create("./res/models/plane.obj");
    //auto model2 = Model::create("./res/models/nanosuit/nanosuit.obj");
    //entity2->add_component(model2);

    auto entity1 = Entity::create("Plane");
    entity1->transform->set_position(hlsl::float3(-20.0f, 0.0f, -20.0f));
    entity1->transform->set_scale(hlsl::float3(22, 0.0f, 22));
    entity1->transform->set_scale(hlsl::float3(1000, 0.0f, 
     1000));
    auto model = Model::create("./res/models/plane.obj");
    entity1->add_component(model);
    auto entity = Entity::create("Grass");
    auto grass = new Grass();
    entity->add_component(grass);
}

