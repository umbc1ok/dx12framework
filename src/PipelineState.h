#pragma once
#include <d3dx12.h>


class Shader;

struct PipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_VS VS;
    CD3DX12_PIPELINE_STATE_STREAM_PS PS;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blendDesc;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL depthStencildesc;
};


class PipelineState
{
public:
    PipelineState(std::string vs_name, std::string ps_name);
    ~PipelineState();
    ID3D12RootSignature* get_root_signature() const;
    ID3D12PipelineState* get_pipeline_state() const;
private:
    ID3D12RootSignature* m_root_signature;
    ID3D12PipelineState* m_pipeline_state;
    void create_root_signature();

    Shader* vertex_shader;
    Shader* pixel_shader;
};
