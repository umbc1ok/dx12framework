#include "Model.h"

#include <DirectXMath.h>
#include <filesystem>
#include <imgui.h>
#include <iostream>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "ResourceLoaders/TextureLoader.h"
#include "utils/Types.h"
#include "Mesh.h"
#include "Renderer.h"
#include "Camera.h"

using namespace DirectX;

Model* Model::create(std::string const& model_path)
{
    Model* model = new Model();

    model->load_model(model_path);
    return model;
}

void Model::set_constant_buffer()
{
    auto commandList = Renderer::get_instance()->g_pd3dCommandList;

    hlsl::float4x4 view = Camera::get_main_camera()->get_view_matrix();
    hlsl::float4x4 projection = Camera::get_main_camera()->get_projection_matrix();
    float time = ImGui::GetTime();
    hlsl::float4x4 world = hlsl::ComposeMatrix(hlsl::float3(0.0f, 0.0f, 0.0f), hlsl::EulerToQuaternion(hlsl::float3(0, 0, 0)), hlsl::float3(0.3f, 0.3f, 0.3f));
    hlsl::float4x4 mvpMatrix = projection * view;
    mvpMatrix = mvpMatrix * world;
    commandList->SetGraphicsRoot32BitConstants(0, 16, &mvpMatrix, 0);
}

void Model::draw()
{
    set_constant_buffer();
    for (auto const& mesh : m_meshes)
    {
        mesh->draw();
    }
}

void Model::load_model(std::string const& path)
{
    Assimp::Importer importer;
    aiScene const* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
    {
        std::cout << "Error. Failed loading a model: " << importer.GetErrorString() << "\n";
        return;
    }

    std::filesystem::path const filesystem_path = path;
    m_directory = filesystem_path.parent_path().string();

    proccess_node(scene->mRootNode, scene);
}

void Model::proccess_node(aiNode const* node, aiScene const* scene)
{
    for (u32 i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh const* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.emplace_back(proccess_mesh(mesh, scene));
    }

    for (u32 i = 0; i < node->mNumChildren; ++i)
    {
        proccess_node(node->mChildren[i], scene);
    }
}

Mesh* Model::proccess_mesh(aiMesh const* mesh, aiScene const* scene)
{
    std::vector<Vertex> vertices;
    std::vector<u16> indices;
    std::vector<Texture*> textures;

    for (u32 i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex = {};

        vertex.position = hlsl::float3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals())
        {
            vertex.normal = hlsl::float3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }

        if (mesh->mTextureCoords[0] != nullptr)
        {
            vertex.UV = hlsl::float2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertex.UV = hlsl::float2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    for (u32 i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace const face = mesh->mFaces[i];
        for (u32 k = 0; k < face.mNumIndices; k++)
        {
            indices.push_back(face.mIndices[k]);
        }
    }

    aiMaterial const* assimp_material = scene->mMaterials[mesh->mMaterialIndex];

    std::vector<Texture*> diffuse_maps =
        load_material_textures(assimp_material, aiTextureType_DIFFUSE, TextureType::Diffuse);
    textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());

    return new Mesh(vertices, indices, textures);
}

std::vector<Texture*> Model::load_material_textures(aiMaterial const* material, aiTextureType const type,
    TextureType const type_name)
{
    std::vector<Texture*> textures;

    u32 const material_count = material->GetTextureCount(type);
    for (u32 i = 0; i < material_count; ++i)
    {
        aiString str;
        material->GetTexture(type, i, &str);

        bool is_already_loaded = false;
        for (auto const& loaded_texture : m_loaded_textures)
        {
            if (std::strcmp(loaded_texture->path.data(), str.C_Str()) == 0)
            {
                textures.push_back(loaded_texture);
                is_already_loaded = true;
                break;
            }
        }

        if (is_already_loaded)
            continue;

        auto file_path = std::string(str.C_Str());
        file_path = m_directory + '/' + file_path;


        Texture* texture = TextureLoader::texture_from_file(file_path);
        textures.push_back(texture);
        m_loaded_textures.push_back(texture);
    }

    return textures;
}