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
#include <Windows.ApplicationModel.DataTransfer.ShareTarget.h>
#include <utils/ErrorHandler.h>
#include <random>

#include "Input.h"
using namespace Microsoft::WRL;

using namespace DirectX;

template <typename T, typename U>
constexpr T DivRoundUp(T num, U denom)
{
    return (num + denom - 1) / denom;
}

Model* Model::create(std::string const& model_path)
{
    Model* model = new Model();

    model->load_model(model_path);
    model->set_can_tick(true);
    model->create_CBV();
    model->m_pipeline_state = new PipelineState(L"MS_MESHOPT.hlsl", L"PS_BASIC.hlsl");
    return model;
}

void Model::set_constant_buffer()
{
    auto commandList = Renderer::get_instance()->g_pd3dCommandList;

    hlsl::float4x4 view = Camera::get_main_camera()->get_view_matrix();
    hlsl::float4x4 projection = Camera::get_main_camera()->get_projection_matrix();
    hlsl::float4x4 world = entity->transform->get_model_matrix();
    hlsl::float4x4 mvpMatrix = projection * view;
    mvpMatrix = mvpMatrix * world;

    m_constant_buffer_data.World = hlsl::transpose(world);
    m_constant_buffer_data.WorldView = hlsl::transpose(world * view);
    m_constant_buffer_data.WorldViewProj = hlsl::transpose(mvpMatrix);
    m_constant_buffer_data.DrawFlag = Renderer::get_instance()->get_debug_mode();

    memcpy(m_cbv_data_begin + sizeof(SceneConstantBuffer) * Renderer::get_instance()->frame_index, &m_constant_buffer_data, sizeof(m_constant_buffer_data));
    commandList->SetGraphicsRootConstantBufferView(0, m_constant_buffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * Renderer::get_instance()->frame_index);
}

void Model::create_CBV()
{
    int frameCount = 3;
    const UINT64 constantBufferSize = sizeof(SceneConstantBuffer) * frameCount;

    const CD3DX12_HEAP_PROPERTIES constantBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

    AssertFailed(Renderer::get_instance()->get_device()->CreateCommittedResource(
        &constantBufferHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &constantBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constant_buffer)));

    // Map and initialize the constant buffer. We don't unmap this until the
    // app closes. Keeping things mapped for the lifetime of the resource is okay.
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.

    AssertFailed(m_constant_buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbv_data_begin)));
    uploadGPUResources();
}

void Model::draw()
{
    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    auto kb = Input::get_instance()->m_keyboard->GetState();
    if (kb.F5)
    {
        m_pipeline_state = new PipelineState(L"MS_MESHOPT.hlsl", L"PS_BASIC.hlsl");
    }
    cmd_list->SetGraphicsRootSignature(m_pipeline_state->get_root_signature());
    cmd_list->SetPipelineState(m_pipeline_state->get_pipeline_state());
    set_constant_buffer();
    for (auto& mesh : m_meshes)
    {
        mesh->dispatch();
    }
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
            //m_type = AddAttribute(m_type, Attribute::Position);
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
        //gowno
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
    return new Mesh(vertices, indices, textures, positions, normals, UVs, attributes);
}

void Model::uploadGPUResources()
{
    auto device = Renderer::get_instance()->get_device();
    auto cmdQueue = Renderer::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    for (uint32_t i = 0; i < m_meshes.size(); ++i)
    {
        auto cmdList = cmdQueue->get_command_list();
        auto& m = m_meshes[i];

        // Create committed D3D resources of proper sizes
        auto indexDesc = CD3DX12_RESOURCE_DESC::Buffer(m->m_indices.size() * sizeof(u32));
        auto meshletDesc = CD3DX12_RESOURCE_DESC::Buffer(m->m_meshlets.size() * sizeof(m->m_meshlets[0]));
        auto cullDataDesc = CD3DX12_RESOURCE_DESC::Buffer(m->m_cullData.size() * sizeof(m->m_cullData[0]));
        // gowno
        auto vertexIndexDesc = CD3DX12_RESOURCE_DESC::Buffer(m->m_uniqueVertexIndices.size());
        auto primitiveDesc = CD3DX12_RESOURCE_DESC::Buffer(m->m_meshletTriangles.size());
        auto meshInfoDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(MeshInfo));

        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        AssertFailed(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &indexDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m->IndexResource)));
        AssertFailed(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &meshletDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m->MeshletResource)));
        AssertFailed(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &primitiveDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m->MeshletTriangleIndicesResource)));


        m->IBView.BufferLocation = m->IndexResource->GetGPUVirtualAddress();
        m->mesh_info.IndexSize = sizeof(u32);
        m->IBView.Format = m->mesh_info.IndexSize == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        m->IBView.SizeInBytes = m->m_indices.size() * m->mesh_info.IndexSize;
        // gowno gowno gowno
        m->VertexResources.resize(1);
        m->VBViews.resize(1);

        //for (uint32_t j = 0; j < m->m_meshlets.size(); ++j)
        {
            auto vertexDesc = CD3DX12_RESOURCE_DESC::Buffer(m->m_vertices.size() * sizeof(Vertex));
            device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &vertexDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m->VertexResources[0]));

            m->VBViews[0].BufferLocation = m->VertexResources[0]->GetGPUVirtualAddress();
            // this line is weird in the other implementation, check if that makes any sense at all
            m->VBViews[0].SizeInBytes = m->m_vertices.size() * sizeof(Vertex);
            m->VBViews[0].StrideInBytes = sizeof(Vertex);
        }

        // Create upload resources
        std::vector<ComPtr<ID3D12Resource>> vertexUploads;
        ComPtr<ID3D12Resource>              indexUpload;
        ComPtr<ID3D12Resource>              meshletUpload;
        ComPtr<ID3D12Resource>              cullDataUpload;
        ComPtr<ID3D12Resource>              uniqueVertexIndexUpload;
        ComPtr<ID3D12Resource>              primitiveIndexUpload;
        ComPtr<ID3D12Resource>              meshInfoUpload;

        auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        AssertFailed(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &indexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexUpload)));
        AssertFailed(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &meshletDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&meshletUpload)));
        AssertFailed(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &primitiveDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&primitiveIndexUpload)));

        // Map & copy memory to upload heap
        vertexUploads.resize(1);
        for (uint32_t j = 0; j < 1; ++j)
        {
            auto vertexDesc = CD3DX12_RESOURCE_DESC::Buffer(m->m_vertices.size() * sizeof(Vertex));
            AssertFailed(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vertexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexUploads[j])));

            uint8_t* memory = nullptr;
            vertexUploads[j]->Map(0, nullptr, reinterpret_cast<void**>(&memory));
            /// KURWAAA TUTAJ NIE MESHLETS TYLKO VERTICES
            std::memcpy(memory, &m->m_vertices[j], m->m_vertices.size() * sizeof(Vertex));
            vertexUploads[j]->Unmap(0, nullptr);
        }

        {
            uint8_t* memory = nullptr;
            indexUpload->Map(0, nullptr, reinterpret_cast<void**>(&memory));
            std::memcpy(memory, m->m_indices.data(), m->m_indices.size() * sizeof(u32));
            indexUpload->Unmap(0, nullptr);
        }

        {
            uint8_t* memory = nullptr;
            meshletUpload->Map(0, nullptr, reinterpret_cast<void**>(&memory));
            std::memcpy(memory, m->m_meshlets.data(), m->m_meshlets.size() * sizeof(m->m_meshlets[0]));
            meshletUpload->Unmap(0, nullptr);
        }

        {
            uint8_t* memory = nullptr;
            primitiveIndexUpload->Map(0, nullptr, reinterpret_cast<void**>(&memory));
            std::memcpy(memory, m->m_meshletTriangles.data(), m->m_meshletTriangles.size());
            primitiveIndexUpload->Unmap(0, nullptr);
        }

        // Populate our command list
        cmdQueue->flush();

        //for (uint32_t j = 0; j < m->m_meshlets.size(); ++j)
        {
            cmdList->CopyResource(m->VertexResources[0].Get(), vertexUploads[0].Get());
            const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m->VertexResources[0].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            cmdList->ResourceBarrier(1, &barrier);
        }

        D3D12_RESOURCE_BARRIER postCopyBarriers[3];

        cmdList->CopyResource(m->IndexResource.Get(), indexUpload.Get());
        postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m->IndexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        cmdList->CopyResource(m->MeshletResource.Get(), meshletUpload.Get());
        postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m->MeshletResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        cmdList->CopyResource(m->MeshletTriangleIndicesResource.Get(), primitiveIndexUpload.Get());
        postCopyBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m->MeshletTriangleIndicesResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        cmdList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);


        cmdQueue->execute_command_list(cmdList);

        auto fence_value = cmdQueue->signal();
        cmdQueue->wait_for_fence_value(fence_value);
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