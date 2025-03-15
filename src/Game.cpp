#include "Game.h"

#include "Entity.h"
#include "Model.h"

void Game::init()
{
    auto entity1 = Entity::create("Bunny");
    //auto model = Model::create("./res/models/plane.obj");

    // SKULL
    //auto model = Model::create("./res/models/skull/skull.fbx");

    //auto model = Model::create("./res/models/lucy/lucy.ply");
    // DRAGON
    //auto model = Model::create("./res/models/dragon/dragon.ply");
    //entity1->transform->set_local_scale(hlsl::float3(0.1f, 0.1f, 0.1f));



    // LUCY
    //entity1->transform->set_local_scale(hlsl::float3(0.1f, 0.1f, 0.1f));
    //entity1->transform->set_local_position(hlsl::float3(-80.0f, 0.0f, 0));
    //entity1->transform->set_rotation(hlsl::float3(-90.0f, 0.0f, 0.0f));

    // buddha setup
    //auto model = Model::create("./res/models/buddha/buddha.fbx");
    //entity1->transform->set_local_scale(hlsl::float3(100.0f, 100.0f, 100.0f));
    //entity1->transform->set_local_position(hlsl::float3(0, -15.0f, 0));



    // bunny setup
    //auto model = Model::create("./res/models/bunny/bunny.fbx");
    //entity1->transform->set_rotation(hlsl::float3(90.0f, 0.0f, 0.0f));


    auto model = Model::create("./res/models/sponza/sponza-gltf-pbr/sponza.glb");
    //auto model = Model::create("./res/models/wing/wing.stl");
    entity1->add_component(model);

    //auto entity2 = Entity::create("House");
    ////auto model = Model::create("./res/models/plane.obj");
    //auto model2 = Model::create("./res/models/pyramid3/scene.gltf");
    //entity2->add_component(model2);

    //auto entity1 = Entity::create("Plane");
    //entity1->transform->set_position(hlsl::float3(20.0f, 0.0f, 20.0f));
    //entity1->transform->set_scale(hlsl::float3(22, 0.0f, 22));
    //entity1->transform->set_scale(hlsl::float3(1000, 0.0f, 
    // 1000));
    //auto model = Model::create("./res/models/plane.obj");
    //entity1->add_component(model);
    //auto entity = Entity::create("Grass");
    //auto grass = new Grass();
    //entity->add_component(grass);
}

