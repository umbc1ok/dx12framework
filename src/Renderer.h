#pragma once
#include <d3dx12.h>

class Renderer
{
public:
    static void update_buffer_resource(ID3D12GraphicsCommandList2* commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
};

