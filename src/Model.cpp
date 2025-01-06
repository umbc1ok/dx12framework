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
#include <d3d12.h>
#include <d3dx12.h>
#include <random>

#include "Input.h"
#include "Serialization/MeshSerializer.h"
#include "utils/Utils.h"
using namespace Microsoft::WRL;

using namespace DirectX;


Model* Model::create(std::string const& model_path)
{

    Model* model = new Model();
    model->m_path = model_path;
    if (!model->deserializeMeshes())
    {
        model->load_model(model_path);
        model->serializeMeshes();
    }
    model->set_can_tick(true);
    model->uploadGPUResources();
    model->m_pipeline_state = new PipelineState(L"MS_STANDARD.hlsl", L"PS_BASIC.hlsl");
    model->m_constant_buffer = new ConstantBuffer<SceneConstantBuffer>();
    model->serializeMeshes();
    return model;
}

void Model::set_constant_buffer()
{
    hlsl::float4x4 view = Camera::get_main_camera()->get_view_matrix();
    hlsl::float4x4 projection = Camera::get_main_camera()->get_projection_matrix();
    hlsl::float4x4 world = entity->transform->get_model_matrix();
    hlsl::float4x4 mvpMatrix = projection * view;
    mvpMatrix = mvpMatrix * world;

    m_constant_buffer_data.World = hlsl::transpose(world);
    m_constant_buffer_data.WorldView = hlsl::transpose(world * view);
    m_constant_buffer_data.WorldViewProj = hlsl::transpose(mvpMatrix);
    m_constant_buffer_data.DrawFlag = Renderer::get_instance()->get_debug_mode();

    m_constant_buffer_data.time = ImGui::GetTime();
    m_constant_buffer->uploadData(m_constant_buffer_data);
    m_constant_buffer->setConstantBuffer(0);

}


void Model::draw()
{
    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    auto const entry = Renderer::get_instance()->get_profiler()->startEntry(cmd_list, "Model Draw");
    {
        
        auto kb = Input::get_instance()->m_keyboard->GetState();
        if (kb.F5)
        {
            m_pipeline_state = new PipelineState(L"MS_STANDARD.hlsl", L"PS_BASIC.hlsl");
        }
        cmd_list->SetGraphicsRootSignature(m_pipeline_state->get_root_signature());
        cmd_list->SetPipelineState(m_pipeline_state->get_pipeline_state());
        set_constant_buffer();
        for (auto& mesh : m_meshes)
        {
            mesh->dispatch();
        }
    } Renderer::get_instance()->get_profiler()->endEntry(cmd_list, entry);
}

void Model::update()
{
    Component::update();

    draw();
}

void Model::draw_editor()
{
    Component::draw_editor();
    ImGui::Text("Triangle count: %i", m_triangle_count);
    ImGui::Text("Vertex count: %i", m_vertex_count);
    ImGui::Text("Meshlet count: %i", m_meshlets_count);

    const char* items[] = { "MESHOPTIMIZER", "DXMESH", "GREEDY"};
    {
        ImGui::Separator();
        ImGui::Text("Meshletizer settings:");
        static bool force_reload = false;
        ImGui::Checkbox("FORCE RELOAD", &force_reload);

        if (ImGui::Combo("MESHLET DEBUG MODE", &m_TypeIndex, items, IM_ARRAYSIZE(items)))
        {
            MeshletizerType type = static_cast<MeshletizerType>(m_TypeIndex);
            m_meshes.clear();
            m_vertex_count = 0;
            m_triangle_count = 0;
            m_meshlets_count = 0;
            if (force_reload || !deserializeMeshes())
            {
                load_model(m_path);
                serializeMeshes();
            }
            uploadGPUResources();
        }
    }
}

void Model::serializeMeshes() const
{
    int index = 0;

    u32 hash = olej_utils::murmur_hash(reinterpret_cast<u8 const*>(m_path.data()), m_path.size(), 69);
    for (auto& mesh : m_meshes)
    {
        serializers::serializeMesh(
            mesh->m_vertices,
            mesh->m_indices,
            mesh->m_meshlets,
            mesh->m_meshletTriangles,
            mesh->m_attributes,
            mesh->m_positions,
            mesh->m_normals,
            mesh->m_UVs,
            mesh->m_MeshletMaxVerts,
            mesh->m_MeshletMaxPrims,
            mesh->m_type,
            "../../cache/mesh/" + std::to_string(hash) + "_" + std::to_string(m_TypeIndex) +"_" + std::to_string(index) +  ".mesh");
        index++;
    }
}

bool Model::deserializeMeshes()
{
    u32 hash = olej_utils::murmur_hash(reinterpret_cast<u8 const*>(m_path.data()), m_path.size(), 69);
    int index = 0;
    for(;;)
    {
        std::string path = "../../cache/mesh/" + std::to_string(hash) + "_" + std::to_string(m_TypeIndex) + "_" + std::to_string(index) + ".mesh";
        if (!std::filesystem::exists(path))
        {
            break;
        }
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        std::vector<Meshlet> meshlets;
        std::vector<u32> meshletTriangles;
        std::vector<u32> attributes;
        std::vector<hlsl::float3> positions;
        std::vector<hlsl::float3> normals;
        std::vector<hlsl::float2> UVs;
        int32_t MeshletMaxVerts = 1;
        int32_t MeshletMaxPrims = 1;
        MeshletizerType type;
        serializers::deserializeMesh(
            vertices,
            indices,
            meshlets,
            meshletTriangles,
            attributes,
            positions,
            normals,
            UVs,
            MeshletMaxVerts,
            MeshletMaxPrims,
            type,
            path);

        m_meshes.push_back(new Mesh(vertices, indices, {}, positions, normals, UVs, attributes, type, meshlets, meshletTriangles));
        index++;
    }

    if (m_meshes.empty())
        return false;
    return true;
}

void Model::load_model(std::string const& path)
{
    Assimp::Importer importer;
    aiScene const* scene = importer.ReadFile(path, aiProcess_FlipUVs | aiProcess_ForceGenNormals | aiProcess_JoinIdenticalVertices);

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
        m_meshlets_count += m_meshes.back()->m_meshlets.size();
    }

    for (u32 i = 0; i < node->mNumChildren; ++i)
    {
        proccess_node(node->mChildren[i], scene);
    }
}

Mesh* Model::proccess_mesh(aiMesh const* mesh, aiScene const* scene)
{
    std::vector<Vertex> vertices;
    std::vector<hlsl::float3> positions;
    std::vector<hlsl::float3> normals;
    std::vector<hlsl::float2> UVs;
    std::vector<u32> indices;
    std::vector<Texture*> textures;
    std::vector<u32> attributes;
    for (u32 i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex = {};

        vertex.position = hlsl::float3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals())
        {
            vertex.normal = hlsl::float3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        else
        {
            vertex.normal = hlsl::float3(0.0f, 0.0f, 0.0f);
        }

        if (mesh->mTextureCoords[0] != nullptr)
        {
            vertex.UV = hlsl::float2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertex.UV = hlsl::float2(0.0f, 0.0f);
        }
        
        positions.push_back(vertex.position);
        UVs.push_back(vertex.UV);
        normals.push_back(vertex.normal);
        vertices.push_back(vertex);
    }

    for (u32 i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace const face = mesh->mFaces[i];
        attributes.push_back(mesh->mMaterialIndex);
        for (u32 k = 0; k < face.mNumIndices; k++)
        {
            indices.push_back(face.mIndices[k]);
        }
    }

    aiMaterial const* assimp_material = scene->mMaterials[mesh->mMaterialIndex];

    std::vector<Texture*> diffuse_maps =
        load_material_textures(assimp_material, aiTextureType_DIFFUSE, TextureType::Diffuse);
    textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());

    // STATS //////////////////////////////
    m_vertex_count += vertices.size();
    m_triangle_count += indices.size() / 3;
    ///////////////////////////////////////
    return new Mesh(vertices, indices, textures, positions, normals, UVs, attributes, static_cast<MeshletizerType>(m_TypeIndex));
}

void Model::uploadGPUResources()
{
    for (uint32_t i = 0; i < m_meshes.size(); ++i)
    {
        auto& m = m_meshes[i];
        if (m->m_indices.size() != 0)
        {
            m->IndexResource = new Resource();
            m->IndexResource->create(m->m_indices.size() * sizeof(u32), m->m_indices.data());
        }

        if (m->m_meshlets.size() != 0)
        {
            m->MeshletResource = new Resource();
            m->MeshletResource->create(m->m_meshlets.size() * sizeof(m->m_meshlets[0]), m->m_meshlets.data());
        }
        if (m->m_meshletTriangles.size() != 0)
        {
            m->MeshletTriangleIndicesResource = new Resource();
            m->MeshletTriangleIndicesResource->create(m->m_meshletTriangles.size() * sizeof(m->m_meshletTriangles[0]), m->m_meshletTriangles.data());
        }

        if (m->m_vertices.size() != 0)
        {
            m->VertexResource = new Resource();
            m->VertexResource->create(m->m_vertices.size() * sizeof(Vertex), m->m_vertices.data());
        }
    }
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


        //Texture* texture = TextureLoader::texture_from_file(file_path);
        //textures.push_back(texture);
        //m_loaded_textures.push_back(texture);
    }

    return textures;
}