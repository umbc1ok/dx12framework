

#include <string>

#include "Mesh.h"

class ResourceManager final
{
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    static void create();

    static ResourceManager* getInstance();
    void scheduleMeshForDeletion(Mesh* meshToDelete);

    void deleteScheduled();

private:
    static ResourceManager* m_instance;

    std::vector<Mesh*> m_meshesToDelete;


};
