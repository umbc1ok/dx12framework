#include "Game.h"

#include "Entity.h"
#include "Model.h"

void Game::init()
{
    auto entity = Entity::create("Bunny");
    auto model = Model::create("./res/models/bunny/bunny.obj");
    entity->add_component(model);

    auto entity1 = Entity::create("Nanosuit");
    auto model1 = Model::create("./res/models/nanosuit/nanosuit.obj");
    entity1->add_component(model1);

}

