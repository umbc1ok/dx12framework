#pragma once
#include <cstdint>
#include <d3d12.h>
#include "Renderer.h"
#include "utils/ErrorHandler.h"
#include <d3dx12.h>



template <typename T>
class ConstantBuffer
{
public:
    ConstantBuffer() = delete;
    ConstantBuffer(const std::string& name);
    ~ConstantBuffer();

    void setConstantBuffer(PipelineState* pso) const;
    void uploadData(const T& data);

private:
    uint8_t* m_cbv_data_begin = nullptr;
    ID3D12Resource* m_d3d12_constant_buffer;

    std::string m_name;
};


template <typename T>
ConstantBuffer<T>::ConstantBuffer(const std::string& name)
{
    int frameCount = 3;
    const UINT64 constantBufferSize = sizeof(T) * frameCount;

    const CD3DX12_HEAP_PROPERTIES constantBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

    AssertFailed(Renderer::get_instance()->get_device()->CreateCommittedResource(
        &constantBufferHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &constantBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_d3d12_constant_buffer)));

    // Map and initialize the constant buffer. We don't unmap this until the
    // app closes. Keeping things mapped for the lifetime of the resource is okay.
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.

    AssertFailed(m_d3d12_constant_buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbv_data_begin)));

    m_name = name;
}

template <typename T>
ConstantBuffer<T>::~ConstantBuffer()
{
    m_d3d12_constant_buffer->Unmap(0, nullptr);
    m_cbv_data_begin = nullptr;
    m_d3d12_constant_buffer->Release();
}

template <typename T>
void ConstantBuffer<T>::setConstantBuffer(PipelineState* pso) const
{
    auto commandList = Renderer::get_instance()->g_pd3dCommandList;
    int32_t index = pso->getRootParameterIndex(m_name);
    if (index == -1)
        return;

    commandList->SetGraphicsRootConstantBufferView(index, m_d3d12_constant_buffer->GetGPUVirtualAddress() + sizeof(T) * Renderer::get_instance()->frame_index);
}

template <typename T>
void ConstantBuffer<T>::uploadData(const T& data)
{
    memcpy(m_cbv_data_begin + sizeof(T) * Renderer::get_instance()->frame_index, &data, sizeof(data));
}

