#include "ResourceManager.h"


ResourceManager* ResourceManager::m_instance;

void ResourceManager::create()
{
    m_instance = new ResourceManager();
}

ResourceManager* ResourceManager::getInstance()
{
    return m_instance;
}

void ResourceManager::scheduleMeshForDeletion(Mesh* meshToDelete)
{
    m_meshesToDelete.push_back(meshToDelete);
}

void ResourceManager::deleteScheduled()
{
    for (Mesh* mesh : m_meshesToDelete)
    {
        delete mesh;
    }

    m_meshesToDelete.clear();
}


