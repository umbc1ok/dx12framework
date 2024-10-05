#pragma once

#include "Texture.h"
#include "assimp/scene.h"
#include "spdlog/details/synchronous_factory.h"
#include "Component.h"

class Mesh;

class Model : public Component
{
public:
    Model() = default;
    static Model* create(std::string const& model_path);
    ~Model() = default;

    void set_constant_buffer();
    void draw();

private:
    void load_model(std::string const& model_path);
    void proccess_node(aiNode const* node, aiScene const* scene);
    Mesh* proccess_mesh(aiMesh const* mesh, aiScene const* scene);
    std::vector<Texture*> load_material_textures(aiMaterial const* material, aiTextureType type, TextureType type_name);

    std::vector<Mesh*> m_meshes;
    std::string m_directory;
    std::vector<Texture*> m_loaded_textures;
};

