#pragma once
#include "DrawableCube.h"


class PipelineState;

class Renderer
{
public:
    Renderer();
    ~Renderer() = default;
    static void create();
    void end_frame();
    void render();
    static void update_buffer_resource(ID3D12GraphicsCommandList2* commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    static Renderer* get_instance();
    static void TransitionResource(ID3D12GraphicsCommandList2* commandList, ID3D12Resource* resource,
        D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
    void ClearRTV(ID3D12GraphicsCommandList2* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);
    void ClearDepth(ID3D12GraphicsCommandList2* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 0.1f);

private:
    DrawableCube* cube;
    PipelineState* m_pipeline_state;
    D3D12_VIEWPORT m_Viewport = {};
    D3D12_RECT m_ScissorRect;
    static Renderer* m_instance;
};

