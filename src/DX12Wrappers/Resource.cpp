#include "Resource.h"

#include <d3dx12.h>

#include "Renderer.h"
#include "utils/ErrorHandler.h"

Resource::Resource(ID3D12Resource* dx12Resource)
{
    m_dx12Resource = dx12Resource;
}

Resource::~Resource()
{
    m_dx12Resource->Release();
}


void Resource::transitionResource(D3D12_RESOURCE_STATES newState)
{
    if (m_currentState == newState) return;

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_dx12Resource,
        m_currentState, newState);
    m_currentState = newState;

    auto commandList = Renderer::get_instance()->g_pd3dCommandList;
    commandList->ResourceBarrier(1, &barrier);
}

void Resource::create(uint64_t size, void* data)
{
    auto device = Renderer::get_instance()->get_device();
    auto cmdQueue = Renderer::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto cmdList = cmdQueue->get_command_list();

    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    AssertFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_dx12Resource)));

    ID3D12Resource* uploadResource;
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    AssertFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadResource)));

    {
        uint8_t* memory = nullptr;
        uploadResource->Map(0, nullptr, reinterpret_cast<void**>(&memory));
        std::memcpy(memory, data, size);
        uploadResource->Unmap(0, nullptr);
    }

    cmdQueue->flush();

    D3D12_RESOURCE_BARRIER postCopyBarrier;

    cmdList->CopyResource(m_dx12Resource, uploadResource);
    postCopyBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_dx12Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cmdList->ResourceBarrier(1, &postCopyBarrier);

    cmdQueue->execute_command_list(cmdList);

    auto fence_value = cmdQueue->signal();
    cmdQueue->wait_for_fence_value(fence_value);
    uploadResource->Release();
}

void Resource::createTexture(D3D12_RESOURCE_DESC descriptor)
{
    auto device = Renderer::get_instance()->get_device();
    auto cmdQueue = Renderer::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto cmdList = cmdQueue->get_command_list();

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);


    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = descriptor.Format;
    FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
    clearValue.Color[0] = clearColor[0];
    clearValue.Color[1] = clearColor[1];
    clearValue.Color[2] = clearColor[2];
    clearValue.Color[3] = clearColor[3];

    AssertFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &descriptor, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&m_dx12Resource)));

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_dx12Resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);

    cmdQueue->flush();

    auto fence_value = cmdQueue->signal();
    cmdQueue->wait_for_fence_value(fence_value);
}

void Resource::bindResource(PipelineState* pso, std::string variableName)
{
    int32_t index = pso->getRootParameterIndex(variableName);

    if (index == -1)
        return;

    auto cmd_list = Renderer::get_instance()->g_pd3dCommandList;
    cmd_list->SetGraphicsRootShaderResourceView(index, m_dx12Resource->GetGPUVirtualAddress());
}
