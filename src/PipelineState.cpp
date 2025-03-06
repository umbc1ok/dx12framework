#include "PipelineState.h"
#include <D3dx12.h>

#include "Renderer.h"
#include "Shader.h"
#include "Window.h"
#include "utils/ErrorHandler.h"
#include "PSOParser.h"
#include "utils/Utils.h"

PipelineState::PipelineState(std::wstring ms_or_vs_name, std::wstring ps_name, PipelineType type)
{
    if (type == TRADITIONAL)
    {
        m_vsName = ms_or_vs_name;
        m_psName = ps_name;
    }
    else if (type == MESH)
    {
        m_msName = ms_or_vs_name;
        m_psName = ps_name;
    }
    m_type = type;
    compilePSO();
    Renderer::get_instance()->register_pipeline_state(this);
}

PipelineState::PipelineState(std::wstring as_name, std::wstring vs_name, std::wstring ps_name)
{
    m_asName = as_name;
    m_msName = vs_name;
    m_psName = ps_name;
    m_type = MESH;
    compilePSO();
    Renderer::get_instance()->register_pipeline_state(this);
}

void PipelineState::compilePSO()
{
    auto device = Renderer::get_instance()->get_device();

    if(!m_asName.empty())
        m_amplificationShader = new Shader(m_asName, ShaderType::AMPLIFICATION);

    if (m_type == MESH)
        m_meshShader = new Shader(m_msName, ShaderType::MESH);
    else
        m_vertexShader = new Shader(m_vsName, ShaderType::VERTEX);
    m_pixelShader = new Shader(m_psName, ShaderType::PIXEL);

    const D3D12_INPUT_ELEMENT_DESC input_layout[3] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };


    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};

    if (m_wireframeActive)
        rasterizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
    else
        rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID; // Use D3D12_FILL_MODE_WIREFRAME for wireframe mode

    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;  // Use D3D12_CULL_MODE_NONE or D3D12_CULL_MODE_FRONT as needed
    rasterizer_desc.FrontCounterClockwise = FALSE;
    rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.MultisampleEnable = FALSE;
    rasterizer_desc.AntialiasedLineEnable = TRUE;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    CD3DX12_BLEND_DESC blend_desc;
    blend_desc = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    blend_desc.RenderTarget[0].BlendEnable = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_SUBTRACT;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blend_desc.AlphaToCoverageEnable = TRUE;
    blend_desc.IndependentBlendEnable = TRUE;

    D3D12_DEPTH_STENCIL_DESC depth_stencil_desc = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());

    createRootSignature();

    if(m_type == MESH)
    {
        D3D12_SHADER_BYTECODE* as = nullptr;
        if (!m_asName.empty())
            as = new D3D12_SHADER_BYTECODE(m_amplificationShader->getBlob()->GetBufferPointer(), m_amplificationShader->getBlob()->GetBufferSize());

        D3D12_SHADER_BYTECODE* ms = new D3D12_SHADER_BYTECODE(m_meshShader->getBlob()->GetBufferPointer(), m_meshShader->getBlob()->GetBufferSize());
        D3D12_SHADER_BYTECODE* ps = new D3D12_SHADER_BYTECODE(m_pixelShader->getBlob()->GetBufferPointer(), m_pixelShader->getBlob()->GetBufferSize());

        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignature;
        psoDesc.MS = *ms;
        psoDesc.PS = *ps;
        if (as)
            psoDesc.AS = *as;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(rasterizer_desc);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.SampleDesc = DefaultSampleDesc();
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;


        auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
        streamDesc.pPipelineStateSubobjectStream = &psoStream;
        streamDesc.SizeInBytes = sizeof(psoStream);

        AssertFailed(device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pipelineState)));
    }
    else if(m_type == TRADITIONAL)
    {
        D3D12_SHADER_BYTECODE* vs = new D3D12_SHADER_BYTECODE(m_vertexShader->getBlob()->GetBufferPointer(), m_vertexShader->getBlob()->GetBufferSize());
        D3D12_SHADER_BYTECODE* ps = new D3D12_SHADER_BYTECODE(m_pixelShader->getBlob()->GetBufferPointer(), m_pixelShader->getBlob()->GetBufferSize());

        PipelineStateStream pipeline_state_stream = {};

        pipeline_state_stream.pRootSignature = m_rootSignature;
        pipeline_state_stream.InputLayout = { input_layout, _countof(input_layout) };
        pipeline_state_stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipeline_state_stream.VS = *vs;
        pipeline_state_stream.PS = *ps;
        pipeline_state_stream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pipeline_state_stream.RTVFormats = rtv_formats;
        pipeline_state_stream.RasterizerState = rasterizer_desc;
        pipeline_state_stream.blendDesc = blend_desc;


        D3D12_PIPELINE_STATE_STREAM_DESC pipeline_state_stream_desc = {
        sizeof(PipelineStateStream), &pipeline_state_stream
        };


        AssertFailed(Renderer::get_instance()->get_device()->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&m_pipelineState)));
    }

}

ID3D12RootSignature* PipelineState::dx12RootSignature() const
{
    return m_rootSignature;
}

ID3D12PipelineState* PipelineState::PSO() const
{
    return m_pipelineState;
}

int32_t PipelineState::getRootParameterIndex(std::string name) const
{
    PSOParser::NameHash hash = olej_utils::murmurHash(reinterpret_cast<const u8*>(name.data()), name.size(), 69);
    if(m_rootParameterMap.find(hash) != m_rootParameterMap.end())
        return m_rootParameterMap.at(hash);

    return -1;
}



void PipelineState::setWireframe(const bool& wireframe)
{
    if(wireframe != m_wireframeActive)
    {
        m_wireframeActive = wireframe;
        compilePSO();
    }
}

void PipelineState::createRootSignature()
{
    std::vector<Shader*> usedShaders;
    if(m_vertexShader != nullptr)
        usedShaders.push_back(m_vertexShader);
    if (m_pixelShader != nullptr)
        usedShaders.push_back(m_pixelShader);
    if (m_meshShader != nullptr)
        usedShaders.push_back(m_meshShader);
    if (m_amplificationShader != nullptr)
        usedShaders.push_back(m_amplificationShader);


    m_rootSignature = PSOParser::parseRootSignature(usedShaders, m_rootParameterMap);
}
