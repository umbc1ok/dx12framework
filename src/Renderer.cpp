#include "Renderer.h"

#include "Window.h"
#include "utils/ErrorHandler.h"

void Renderer::update_buffer_resource(ID3D12GraphicsCommandList2* commandList,
    ID3D12Resource** pDestinationResource,
    ID3D12Resource** pIntermediateResource,
    size_t numElements, size_t elementSize, const void* bufferData,
    D3D12_RESOURCE_FLAGS flags)
{
    auto device = Window::get_instance()->get_device();

    size_t bufferSize = numElements * elementSize;

    // Create a committed resource for the GPU resource in a default heap.
    auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    AssertFailed(device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(pDestinationResource)));

    // Create a committed resource for the upload.
    if (bufferData)
    {
        auto upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto buffer_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        AssertFailed(device->CreateCommittedResource(
            &upload_heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &buffer_resource_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(pIntermediateResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        UpdateSubresources(commandList,
            *pDestinationResource, *pIntermediateResource,
            0, 0, 1, &subresourceData);
    }
}
