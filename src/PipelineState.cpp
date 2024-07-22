#include "PipelineState.h"
#include "Shader.h"
#include "Window.h"
#include "utils/ErrorHandler.h"
#include "utils/maths.h"


PipelineState::PipelineState(std::string vs_name, std::string ps_name)
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    vertex_shader = new Shader(vs_name, ShaderType::VERTEX);
    pixel_shader = new Shader(ps_name, ShaderType::PIXEL);

    create_root_signature();
    PipelineStateStream pipeline_state_stream = {};

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;


    CD3DX12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // Use D3D12_FILL_MODE_WIREFRAME for wireframe mode
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;  // Use D3D12_CULL_MODE_NONE or D3D12_CULL_MODE_FRONT as needed
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = FALSE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    CD3DX12_BLEND_DESC blendDesc;
    blendDesc = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    blendDesc.RenderTarget[0].BlendEnable = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.AlphaToCoverageEnable = TRUE;
    blendDesc.IndependentBlendEnable = TRUE;

    CD3DX12_DEPTH_STENCIL_DESC depthStencildesc = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    // VERY FUCKING IMPORTANT LINE
    depthStencildesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;


    pipeline_state_stream.pRootSignature = m_root_signature;
    pipeline_state_stream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipeline_state_stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_state_stream.VS = CD3DX12_SHADER_BYTECODE(vertex_shader->get_blob());
    pipeline_state_stream.PS = CD3DX12_SHADER_BYTECODE(pixel_shader->get_blob());
    pipeline_state_stream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipeline_state_stream.RTVFormats = rtvFormats;
    pipeline_state_stream.RasterizerState = rasterizerDesc;
    pipeline_state_stream.blendDesc = blendDesc;
    pipeline_state_stream.depthStencildesc = depthStencildesc;


    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
    sizeof(PipelineStateStream), &pipeline_state_stream
    };

    auto commandQueue = Window::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->get_command_list();

    AssertFailed(Window::get_instance()->get_device()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pipeline_state)));

    auto fenceValue = commandQueue->execute_command_list(commandList);
    commandQueue->wait_for_fence_value(fenceValue);
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
    auto device = Window::get_instance()->get_device();
    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(hlsl::float4x4) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    // Serialize the root signature.
    ID3DBlob* root_signature_blob;
    ID3DBlob* error_blob;
    AssertFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &root_signature_blob, &error_blob));
    // Create the root signature.
    AssertFailed(device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));

    root_signature_blob->Release();
    //error_blob->Release();
}
