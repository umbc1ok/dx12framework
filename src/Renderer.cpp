#include "Renderer.h"

#include "DrawableCube.h"
#include "PipelineState.h"
#include "Window.h"
#include "utils/ErrorHandler.h"

Renderer* Renderer::m_instance;
Renderer::Renderer()
{
    cube = new DrawableCube();
    m_pipeline_state = new PipelineState("basic.hlsl", "basic.hlsl");
    m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
        static_cast<float>(1920), static_cast<float>(1080));
    m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
}

void Renderer::create()
{
    m_instance = new Renderer();
}
void Renderer::end_frame()
{

}

void Renderer::render()
{
    auto window = Window::get_instance();
    auto cmdqueue = window->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto cmd_list = Window::get_instance()->g_pd3dCommandList;
    auto back_buffer = window->get_current_back_buffer();
    auto rtv = window->get_current_rtv();
    auto dsv = window->get_dsv_heap()->GetCPUDescriptorHandleForHeapStart();
    //TransitionResource(cmd_list, back_buffer,
    //    D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
    FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

    ClearRTV(cmd_list, rtv, clearColor);
    ClearDepth(cmd_list, dsv);

    cmd_list->SetPipelineState(m_pipeline_state->get_pipeline_state());
    cmd_list->SetGraphicsRootSignature(m_pipeline_state->get_root_signature());

    cube->predraw();
    cmd_list->RSSetViewports(1, &m_Viewport);
    cmd_list->RSSetScissorRects(1, &m_ScissorRect);

    cmd_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    cube->draw();
}

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

Renderer* Renderer::get_instance()
{
    return m_instance;
}

void Renderer::TransitionResource(ID3D12GraphicsCommandList2* commandList,
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource,
        beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}

void Renderer::ClearRTV(ID3D12GraphicsCommandList2* commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void Renderer::ClearDepth(ID3D12GraphicsCommandList2* commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}