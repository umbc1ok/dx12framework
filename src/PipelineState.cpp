#include "PipelineState.h"
#include <D3dx12.h>

#include "Renderer.h"
#include "Shader.h"
#include "Window.h"
#include "utils/ErrorHandler.h"
#include "utils/maths.h"

PipelineState::PipelineState(std::string vs_name, std::string ps_name)
{
    D3D12_INPUT_ELEMENT_DESC input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    vertex_shader = new Shader(vs_name, ShaderType::VERTEX);
    pixel_shader = new Shader(ps_name, ShaderType::PIXEL);

    create_root_signature();
    PipelineStateStream pipeline_state_stream = {};

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;


    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID; // Use D3D12_FILL_MODE_WIREFRAME for wireframe mode
    rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;  // Use D3D12_CULL_MODE_NONE or D3D12_CULL_MODE_FRONT as needed
    rasterizer_desc.FrontCounterClockwise = FALSE;
    rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer_desc.DepthClipEnable = FALSE;
    rasterizer_desc.MultisampleEnable = FALSE;
    rasterizer_desc.AntialiasedLineEnable = FALSE;
    rasterizer_desc.ForcedSampleCount = 0;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    CD3DX12_BLEND_DESC blend_desc;
    blend_desc = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    blend_desc.RenderTarget[0].BlendEnable = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blend_desc.AlphaToCoverageEnable = TRUE;
    blend_desc.IndependentBlendEnable = TRUE;

    CD3DX12_DEPTH_STENCIL_DESC depth_stencil_desc = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    // VERY FUCKING IMPORTANT LINE
    depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;


    pipeline_state_stream.pRootSignature = m_root_signature;
    pipeline_state_stream.InputLayout = { input_layout, _countof(input_layout) };
    pipeline_state_stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_state_stream.VS = CD3DX12_SHADER_BYTECODE(vertex_shader->get_blob());
    pipeline_state_stream.PS = CD3DX12_SHADER_BYTECODE(pixel_shader->get_blob());
    pipeline_state_stream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipeline_state_stream.RTVFormats = rtv_formats;
    pipeline_state_stream.RasterizerState = rasterizer_desc;
    pipeline_state_stream.blendDesc = blend_desc;
    pipeline_state_stream.depthStencildesc = depth_stencil_desc;


    D3D12_PIPELINE_STATE_STREAM_DESC pipeline_state_stream_desc = {
    sizeof(PipelineStateStream), &pipeline_state_stream
    };


    AssertFailed(Renderer::get_instance()->get_device()->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&m_pipeline_state)));

}

ID3D12RootSignature* PipelineState::get_root_signature() const
{
    return m_root_signature;
}

ID3D12PipelineState* PipelineState::get_pipeline_state() const
{
    return m_pipeline_state;
}

// This is very basic, using it only to render a cube for now
void PipelineState::create_root_signature()
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
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 root_parameters[2];
    root_parameters[0].InitAsConstants(sizeof(hlsl::float4x4) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_DESCRIPTOR_RANGE1 myTextureDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    root_parameters[1].InitAsDescriptorTable(1, &myTextureDescriptorRange);
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_description;

    CD3DX12_STATIC_SAMPLER_DESC static_samplers[1];
    static_samplers[0].Init(0, D3D12_FILTER_ANISOTROPIC); // s3

    root_signature_description.Init_1_1(_countof(root_parameters), root_parameters, 1, static_samplers, root_signature_flags);

    // Serialize the root signature.
    ID3DBlob* root_signature_blob;
    ID3DBlob* error_blob;
    AssertFailed(D3DX12SerializeVersionedRootSignature(&root_signature_description,
        feature_data.HighestVersion, &root_signature_blob, &error_blob));

    // Create the root signature.
    AssertFailed(device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));

    root_signature_blob->Release();
}
