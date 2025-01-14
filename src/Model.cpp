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

#include "DX12Wrappers/ConstantBuffer.h"
#include "Tools/GPUProfiler.h"
#include "Tools/MeshletBenchmark.h"

using namespace Microsoft::WRL;



Model* Model::create(std::string const& model_path)
{

    Model* model = new Model();
    model->m_path = model_path;
    if (!model->deserializeMeshes())
    {
        model->loadModel(model_path);
        //model->serializeMeshes();
    }
    model->set_can_tick(true);
    model->uploadGPUResources();
    model->m_smallMeshletPipelineState = new PipelineState(L"AS_STANDARD.hlsl", L"MS_STANDARD.hlsl", L"PS_BASIC.hlsl");
    model->m_bigMeshletPipelineState = new PipelineState(L"AS_STANDARD.hlsl", L"MS_BIG.hlsl", L"PS_BASIC.hlsl");
    model->m_sceneConstantBuffer = new ConstantBuffer<SceneConstantBuffer>();
    model->m_cameraConstantBuffer = new ConstantBuffer<CameraConstants>();
    return model;
}

void Model::setConstantBuffer()
{
    hlsl::float4x4 view = Camera::getMainCamera()->getViewMatrix();
    hlsl::float4x4 projection = Camera::getMainCamera()->getProjectionMatrix();
    hlsl::float4x4 world = entity->transform->get_model_matrix();
    hlsl::float4x4 mvpMatrix = projection * view;
    mvpMatrix = mvpMatrix * world;

    m_sceneConstantBufferData.World = hlsl::transpose(world);
    m_sceneConstantBufferData.WorldView = hlsl::transpose(world * view);
    m_sceneConstantBufferData.WorldViewProj = hlsl::transpose(mvpMatrix);
    m_sceneConstantBufferData.DrawFlag = Renderer::get_instance()->get_debug_mode();

    m_sceneConstantBufferData.time = ImGui::GetTime();
    m_sceneConstantBuffer->uploadData(m_sceneConstantBufferData);
    m_sceneConstantBuffer->setConstantBuffer(0);

    m_cameraConstants.CullViewPosition = Camera::getMainCamera()->getCullingPosition();
    auto const frustum = Camera::getMainCamera()->getFrustum();
    m_cameraConstants.Planes[0] = frustum.top_plane;
    m_cameraConstants.Planes[1] = frustum.bottom_plane;
    m_cameraConstants.Planes[2] = frustum.right_plane;
    m_cameraConstants.Planes[3] = frustum.left_plane;
    m_cameraConstants.Planes[4] = frustum.far_plane;
    m_cameraConstants.Planes[5] = frustum.near_plane;
    m_cameraConstantBuffer->uploadData(m_cameraConstants);
    m_cameraConstantBuffer->setConstantBuffer(2);

}


void Model::draw()
{
    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    auto profiler = GPUProfiler::getInstance();

    auto const entry = profiler->startEntry(cmd_list, "Model Draw");
    {
        if(m_MeshletMaxVerts > 128 || m_MeshletMaxPrims > 128)
        {
            cmd_list->SetGraphicsRootSignature(m_bigMeshletPipelineState->dx12RootSignature());
            cmd_list->SetPipelineState(m_bigMeshletPipelineState->PSO());
        }
        else
        {
            cmd_list->SetGraphicsRootSignature(m_smallMeshletPipelineState->dx12RootSignature());
            cmd_list->SetPipelineState(m_smallMeshletPipelineState->PSO());
        }
        setConstantBuffer();
        for (auto& mesh : m_meshes)
        {
            mesh->dispatch();
        }
    } profiler->endEntry(cmd_list, entry);
}

void Model::update()
{
    Component::update();

    draw();
}

void Model::drawEditor()
{
    Component::drawEditor();
    ImGui::Text("Triangle count: %i", m_triangleCount);
    ImGui::Text("Vertex count: %i", m_vertexCount);
    ImGui::Text("Meshlet count: %i", m_meshletsCount);

    const char* items[] = { "MESHOPTIMIZER", "DXMESH", "GREEDY", "BoundingSphere", "NVIDIA"};
    {
        ImGui::Separator();
        ImGui::Text("Meshletizer settings:");

        if (ImGui::Combo("MESHLET DEBUG MODE", &m_TypeIndex, items, IM_ARRAYSIZE(items)))
        {
            MeshletizerType type = static_cast<MeshletizerType>(m_TypeIndex);
            m_meshes.clear();
            m_vertexCount = 0;
            m_triangleCount = 0;
            m_meshletsCount = 0;
            if (!deserializeMeshes())
            {
                loadModel(m_path);
                serializeMeshes();
            }
            uploadGPUResources();
        }
    }

    if(ImGui::Button("Update meshlet benchmark with this Model's metadata"))
    {
        MeshletBenchmark::getInstance()->updateMeshletizerType(static_cast<MeshletizerType>(m_TypeIndex));
        MeshletBenchmark::getInstance()->updateMeshletParameters(m_MeshletMaxVerts, m_MeshletMaxPrims);
    }
    ImGui::InputInt("Max meshlet vertices", &m_MeshletMaxVerts);
    ImGui::InputInt("Max meshlet primitives", &m_MeshletMaxPrims);
    if(ImGui::Button("RELOAD"))
    {
        m_meshes.clear();
        m_vertexCount = 0;
        m_triangleCount = 0;
        m_meshletsCount = 0;
        loadModel(m_path);
        serializeMeshes();
        uploadGPUResources();
    }

}

void Model::serializeMeshes() const
{
    int index = 0;

    u32 hash = olej_utils::murmurHash(reinterpret_cast<u8 const*>(m_path.data()), m_path.size(), 69);
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
            mesh->m_cullData,
            mesh->m_MeshletMaxVerts,
            mesh->m_MeshletMaxPrims,
            mesh->m_type,
            "../../cache/mesh/" + std::to_string(hash) + "_" + std::to_string(m_TypeIndex) +"_" + std::to_string(index) +  ".mesh");
        index++;
    }
}

bool Model::deserializeMeshes()
{
    u32 hash = olej_utils::murmurHash(reinterpret_cast<u8 const*>(m_path.data()), m_path.size(), 69);
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
        std::vector<CullData> cullData;
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
            cullData,
            MeshletMaxVerts,
            MeshletMaxPrims,
            type,
            path);

        m_meshes.push_back(new Mesh(vertices, indices, {}, positions, normals, UVs, attributes, type, m_MeshletMaxVerts, m_MeshletMaxPrims, meshlets, meshletTriangles, cullData));
        index++;
    }

    if (m_meshes.empty())
        return false;
    return true;
}

void Model::loadModel(std::string const& path)
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

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode const* node, aiScene const* scene)
{
    for (u32 i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh const* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.emplace_back(processMesh(mesh, scene));
        m_meshletsCount += m_meshes.back()->m_meshlets.size();
    }

    for (u32 i = 0; i < node->mNumChildren; ++i)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh* Model::processMesh(aiMesh const* mesh, aiScene const* scene)
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
        loadMaterialTextures(assimp_material, aiTextureType_DIFFUSE, TextureType::Diffuse);
    textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());

    // STATS //////////////////////////////
    m_vertexCount += vertices.size();
    m_triangleCount += indices.size() / 3;
    ///////////////////////////////////////
    return new Mesh(vertices, indices, textures, positions, normals, UVs, attributes, static_cast<MeshletizerType>(m_TypeIndex), m_MeshletMaxVerts, m_MeshletMaxPrims);
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

        if (m->m_cullData.size() != 0)
        {
            m->CullDataResource = new Resource();
            m->CullDataResource->create(m->m_cullData.size() * sizeof(CullData), m->m_cullData.data());
        }
    }
}

std::vector<Texture*> Model::loadMaterialTextures(aiMaterial const* material, aiTextureType const type,
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