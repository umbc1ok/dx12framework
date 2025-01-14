#include "Resource.h"

#include <d3dx12.h>

#include "Renderer.h"
#include "utils/ErrorHandler.h"

Resource::~Resource()
{
    m_dx12Resource->Release();
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
