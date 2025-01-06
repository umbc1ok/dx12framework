#pragma once

#include "Texture.h"
#include "assimp/scene.h"
#include "Component.h"
#include "PipelineState.h"
#include "utils/maths.h"
#include "../res/shaders/shared/shared_cb.h"
#include "DX12Wrappers/ConstantBuffer.h"
#include "MeshletStructs.h"

class Mesh;

class Model : public Component
{
public:
    Model() = default;
    static Model* create(std::string const& model_path);
    ~Model() = default;

    void set_constant_buffer();
    void draw();
    void update() override;
    void draw_editor() override;
    void serializeMeshes() const;
    bool deserializeMeshes();

    std::vector<Texture*> m_loaded_textures;
private:
    void load_model(std::string const& model_path);
    void proccess_node(aiNode const* node, aiScene const* scene);
    Mesh* proccess_mesh(aiMesh const* mesh, aiScene const* scene);

    void uploadGPUResources();
    std::vector<Texture*> load_material_textures(aiMaterial const* material, aiTextureType type, TextureType type_name);

    std::vector<Mesh*> m_meshes;
    std::string m_directory;


    ConstantBuffer<SceneConstantBuffer>* m_constant_buffer;
    SceneConstantBuffer m_constant_buffer_data;

    // STATS FOR EDITOR
    int m_vertex_count = 0;
    int m_triangle_count = 0;
    int m_meshlets_count = 0;

    int m_TypeIndex = 2;

    std::string m_path;

    PipelineState* m_pipeline_state;
};

