#include "Game.h"

#include "Entity.h"
#include "../build/src/MeshletizedModel.h"

void Game::init()
{
    auto entity = Entity::create("Dragon");
    auto model = new MeshletizedModel();
    model->LoadFromFile(L"./res/models/dragon/Dragon_LOD0.bin");
    entity->add_component(model);
}

