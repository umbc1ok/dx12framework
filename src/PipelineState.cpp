#include "PipelineState.h"
#include <D3dx12.h>

#include "Renderer.h"
#include "Shader.h"
#include "Window.h"
#include "utils/ErrorHandler.h"
#include "utils/maths.h"

PipelineState::PipelineState(std::wstring vs_name, std::wstring ps_name)
{
    m_msName = vs_name;
    m_asName = L"";
    m_psName = ps_name;
    compilePSO();
    Renderer::get_instance()->register_pipeline_state(this);
}

void PipelineState::compilePSO()
{
    auto device = Renderer::get_instance()->get_device();

    mesh_shader = new Shader(m_msName, ShaderType::MESH);
    pixel_shader = new Shader(m_psName, ShaderType::PIXEL);

    const D3D12_INPUT_ELEMENT_DESC input_layout[3] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 },
        { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 }
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

    //rasterizer_desc.ForcedSampleCount = 0;
    if (m_wireframeActive)
        rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    else
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

    D3D12_SHADER_BYTECODE* ms = new D3D12_SHADER_BYTECODE(mesh_shader->getBlob()->GetBufferPointer(), mesh_shader->getBlob()->GetBufferSize());
    D3D12_SHADER_BYTECODE* ps = new D3D12_SHADER_BYTECODE(pixel_shader->getBlob()->GetBufferPointer(), pixel_shader->getBlob()->GetBufferSize());

    createRootSignature();
    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature;
    psoDesc.MS = *ms;
    psoDesc.PS = *ps;
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

ID3D12RootSignature* PipelineState::dx12RootSignature() const
{
    return m_rootSignature;
}

ID3D12PipelineState* PipelineState::PSO() const
{
    return m_pipelineState;
}

void PipelineState::setWireframe(const bool& wireframe)
{
    if(wireframe != m_wireframeActive)
    {
        m_wireframeActive = wireframe;
        compilePSO();
    }
}

// This is very basic, using it only to render a cube for now
void PipelineState::createRootSignature()
{
    auto device = Renderer::get_instance()->get_device();
    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;


    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_description;

    if (m_msName == L"MS_GRASS.hlsl")
    {
        CD3DX12_ROOT_PARAMETER1 root_parameters[3];
        root_parameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[2].InitAsUnorderedAccessView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);


        CD3DX12_STATIC_SAMPLER_DESC static_samplers[1];
        static_samplers[0].Init(0, D3D12_FILTER_ANISOTROPIC); // s3

        root_signature_description.Init_1_1(_countof(root_parameters), root_parameters, 1, static_samplers, root_signature_flags);
    }
    else
    {
        
        CD3DX12_ROOT_PARAMETER1 root_parameters[6];
        //
        root_parameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[1].InitAsConstants(2, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[2].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[3].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[4].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
        root_parameters[5].InitAsShaderResourceView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);

        CD3DX12_STATIC_SAMPLER_DESC static_samplers[1];
        static_samplers[0].Init(0, D3D12_FILTER_ANISOTROPIC); // s3

        root_signature_description.Init_1_1(_countof(root_parameters), root_parameters, 1, static_samplers, root_signature_flags);
    }



    // Serialize the root signature.
    ID3DBlob* root_signature_blob;
    ID3DBlob* error_blob;
    AssertFailed(D3DX12SerializeVersionedRootSignature(&root_signature_description,
        feature_data.HighestVersion, &root_signature_blob, &error_blob));

    // Create the root signature.
    AssertFailed(device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    root_signature_blob->Release();
}
