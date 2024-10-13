#include "Game.h"

#include "Entity.h"
#include "MeshletizedModel.h"

void Game::init()
{
    auto entity = Entity::create("Dragon");
    auto entity2 = Entity::create("Dragon2");
    auto entity3 = Entity::create("Dragon3");
    auto model = new MeshletizedModel();
    auto model2 = new MeshletizedModel();
    auto model3 = new MeshletizedModel();
    model->LoadFromFile(L"./res/models/dragon/Dragon_LOD0.bin");
    model2->LoadFromFile(L"./res/models/dragon/Dragon_LOD0.bin");
    model3->LoadFromFile(L"./res/models/dragon/Dragon_LOD0.bin");
    entity->add_component(model);
    entity2->add_component(model2);
    entity3->add_component(model3);
    entity2->transform->set_local_position(hlsl::float3(0.0f, 5.0f, 0.0f));
}

