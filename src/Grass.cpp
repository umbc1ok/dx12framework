#include "Grass.h"

#include <d3dx12.h>
#include <imgui.h>

#include "Camera.h"
#include "Input.h"
#include "utils/ErrorHandler.h"


Grass::Grass()
{
    set_can_tick(true);
    m_constant_buffer = new ConstantBuffer<SceneConstantBuffer>();
    m_WindConstantBuffer = new ConstantBuffer<Wind>();
}

void Grass::start()
{
    auto device = Renderer::get_instance()->get_device();
    m_pipeline_state = new PipelineState(L"MS_GRASS.hlsl", L"PS_GRASS.hlsl");

    generate_blades();
    upload_GPU_resources();

}

void Grass::draw_editor()
{
    Component::draw_editor();
    ImGui::SliderFloat2("Wind Direction", &wind->direction.x, -1, 1);
    wind->direction = hlsl::normalize(wind->direction);
    ImGui::SliderFloat("Wind Strength", &m_base_force,0,500);
    ImGui::SliderFloat("Restoration strength factor", &wind->restoration_strength,0, 0.5);

    ImGui::Text("OSCILLATION");
    ImGui::Checkbox("Wind oscillations", &m_oscilateWind);
    ImGui::SliderFloat("Oscilations strength", &m_oscilationsStrength, 0, 100);
    ImGui::SliderFloat("Oscilation delta per frame", &m_oscillationsSpeed, 0, 5);

    ImGui::Text("PULSATION");
    ImGui::Checkbox("Pulsations", &m_pulsateWind);
    ImGui::SliderFloat("Pulsation strength", &m_pulsationStrength, 0, 100);
    ImGui::SliderFloat("Pulsation delta per frame", &m_pulsationSpeed, 0, 5);
}

void Grass::update()
{
    Component::update();
    wind->force = m_base_force;
    if (m_oscilateWind)
    {
        if(m_oscilationRising)
        {
            m_currentOscillation += m_oscillationsSpeed;
            if (m_currentOscillation >= m_oscilationsStrength)
            {
                m_oscilationRising = false;
            }
        }
        else
        {
            m_currentOscillation -= m_oscillationsSpeed;
            if (m_currentOscillation <= -m_oscilationsStrength)
            {
                m_oscilationRising = true;
            }
        }
        wind->force += m_currentOscillation;
    }
    if (m_pulsateWind)
    {
        if (m_pulsationRising)
        {
            m_currentPulsation += m_pulsationSpeed;
            if (m_currentPulsation >= m_pulsationStrength)
            {
                m_pulsationRising = false;
            }
            wind->force += m_currentPulsation;
        }
        else
        {
            m_currentPulsation -= m_pulsationSpeed / 50.0f;
            if (m_currentPulsation <= 0)
            {
                m_pulsationRising = true;
            }
            // we dont add the pulstation here, this else is effectively a cooldown timer
        }
        
    }

    auto kb = Input::get_instance()->m_keyboard->GetState();
    if (kb.F5)
    {
        m_pipeline_state = new PipelineState(L"MS_GRASS.hlsl", L"PS_GRASS.hlsl");
    }

    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    cmd_list->SetGraphicsRootSignature(m_pipeline_state->get_root_signature());
    cmd_list->SetPipelineState(m_pipeline_state->get_pipeline_state());
    dispatch();
}

void Grass::generate_blades()
{
    uint32_t size = 40000; // ROOT OF size must be a natural number
    uint32_t rowsize = sqrt(size);
    m_blades.resize(size);
    srand(time(NULL));

    for (int i = 0; i < rowsize; i++)
    {
        for (int j = 0; j < rowsize; j++)
        {
            Blade b;
            float distanceDivider = 5.0f;
            float pos_randomizerX = rand() % 200 / 200.0f - 1.0f;
            float pos_randomizerZ = rand() % 200 / 200.0f - 1.0f;
            b.m_height = (rand() % 15) / 10.0f + 0.5f;
            b.m_tip_stiffness = (rand() % 5) / 10.0f + 0.1f;
            b.m_root_position = hlsl::float3(i / distanceDivider + pos_randomizerX, 0.0f, j / distanceDivider + pos_randomizerZ);
            b.m_middle_position = hlsl::float3(i / distanceDivider + pos_randomizerX, b.m_height / 2.0f, j / distanceDivider + pos_randomizerZ);
            b.m_tip_position = hlsl::float3(i / distanceDivider + pos_randomizerX, b.m_height, j / distanceDivider + pos_randomizerZ);
            b.movement_speed_middle = hlsl::float3(0.0f, 0.0f, 0.0f);
            b.movement_speed_tip = hlsl::float3(0.0f, 0.0f, 0.0f);
            m_blades[i * rowsize + j] = b;
        }
    }

    wind = new Wind();
    wind->direction = hlsl::float2(1.0f, 0.0f);
    wind->force = 0.1f;
    wind->restoration_strength = 0.35f;

}

void Grass::set_constant_buffer()
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

    m_WindConstantBuffer->uploadData(*wind);
    m_WindConstantBuffer->setConstantBuffer(1);
}

void Grass::dispatch()
{
    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    set_constant_buffer();
    cmd_list->SetGraphicsRootUnorderedAccessView(2, m_bladesResource->GetGPUVirtualAddress());
    cmd_list->DispatchMesh(m_blades.size(), 1, 1);
}

void Grass::upload_GPU_resources()
{
    auto device = Renderer::get_instance()->get_device();
    auto cmdQueue = Renderer::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto blades_desc = CD3DX12_RESOURCE_DESC::Buffer(m_blades.size() * sizeof(Blade));
    blades_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    AssertFailed(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &blades_desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_bladesResource)));

    Microsoft::WRL::ComPtr<ID3D12Resource>              blades_upload;
    blades_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    AssertFailed(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &blades_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&blades_upload)));

    {
        uint8_t* memory = nullptr;
        blades_upload->Map(0, nullptr, reinterpret_cast<void**>(&memory));
        std::memcpy(memory, m_blades.data(), m_blades.size() * sizeof(Blade));
        blades_upload->Unmap(0, nullptr);
    }
    cmdQueue->flush();


    D3D12_RESOURCE_BARRIER postCopyBarrier;
    auto cmdList = cmdQueue->get_command_list();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = static_cast<UINT>(m_blades.size());
    uavDesc.Buffer.StructureByteStride = sizeof(Blade);
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    auto uavHandle = Renderer::get_instance()->get_srv_desc_heap()->GetCPUDescriptorHandleForHeapStart();
    device->CreateUnorderedAccessView(m_bladesResource.Get(), nullptr, &uavDesc, uavHandle);

    // Bind the UAV heap to the command list (for use in shaders)
    ID3D12DescriptorHeap* heaps[] = { Renderer::get_instance()->get_srv_desc_heap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

    cmdList->CopyResource(m_bladesResource.Get(), blades_upload.Get());
    postCopyBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_bladesResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cmdList->ResourceBarrier(1, &postCopyBarrier);
    cmdQueue->execute_command_list(cmdList);
    auto fence_value = cmdQueue->signal();
    cmdQueue->wait_for_fence_value(fence_value);
}
