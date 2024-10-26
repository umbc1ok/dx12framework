#pragma once

#include <wrl/client.h>

#include "Texture.h"
#include "assimp/scene.h"
#include "spdlog/details/synchronous_factory.h"
#include "Component.h"
#include "utils/maths.h"

class Mesh;



_declspec(align(256u)) struct SceneConstantBuffer
{
    hlsl::float4x4 World;
    hlsl::float4x4 WorldView;
    hlsl::float4x4 WorldViewProj;
    uint32_t   DrawMeshlets;
};


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
private:
    void load_model(std::string const& model_path);
    void proccess_node(aiNode const* node, aiScene const* scene);
    Mesh* proccess_mesh(aiMesh const* mesh, aiScene const* scene);
    void create_CBV();


    void uploadGPUResources();
    std::vector<Texture*> load_material_textures(aiMaterial const* material, aiTextureType type, TextureType type_name);

    std::vector<Mesh*> m_meshes;
    std::string m_directory;
    std::vector<Texture*> m_loaded_textures;


    SceneConstantBuffer m_constant_buffer_data;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constant_buffer;
    UINT8* m_cbv_data_begin = nullptr;


    // STATS FOR EDITOR
    int m_vertex_count = 0;
    int m_triangle_count = 0;
    int m_meshlets_count = 0;
};

