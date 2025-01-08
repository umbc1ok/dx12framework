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

struct PipelineStateStreamMesh
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_AS AS;
    CD3DX12_PIPELINE_STATE_STREAM_MS MS;
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
    PipelineState(std::wstring vs_name, std::wstring ps_name);
    PipelineState(std::wstring as_name, std::wstring vs_name, std::wstring ps_name);
    ~PipelineState() = default;

    void compilePSO();
    void setWireframe(const bool& wireframe);

    ID3D12RootSignature* dx12RootSignature() const;
    ID3D12PipelineState* PSO() const;

private:
    void createRootSignature();

    ID3D12RootSignature* m_rootSignature;
    ID3D12PipelineState* m_pipelineState;

    std::wstring m_msName = L"";
    std::wstring m_asName = L"";
    std::wstring m_psName = L"";

    bool m_wireframeActive = false;

    Shader* m_amplificationShader;
    Shader* m_meshShader;
    Shader* m_pixelShader;
};

