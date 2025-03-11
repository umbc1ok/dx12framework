#include "PSOParser.h"

#include <d3dx12.h>
#include <fstream>
#include <iostream>
#include <ostream>
#include <regex>
#include <sstream>
#include <unordered_map>

#include "Renderer.h"
#include "Shader.h"
#include "DXMeshletGenerator/D3D12MeshletGenerator.h"
#include "utils/Utils.h"

namespace PSOParser
{
    
    D3D12_SHADER_VISIBILITY mapShaderTypeToVisibility(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::AMPLIFICATION:
            return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
        case ShaderType::MESH:
            return D3D12_SHADER_VISIBILITY_MESH;
        case ShaderType::PIXEL:
            return D3D12_SHADER_VISIBILITY_PIXEL;
        case ShaderType::VERTEX:
            return D3D12_SHADER_VISIBILITY_VERTEX;
        default:
            return D3D12_SHADER_VISIBILITY_ALL;
        }
    }

    std::vector<ShaderResource> ExtractResources(const std::string& hlslCode, int shaderTypeIndex, ShaderType type)
    {
        std::vector<ShaderResource> resources;

        std::regex resourceRegex(R"((cbuffer|Texture2D|RWTexture2D|SamplerState)\s+(\w+)\s*:\s*register\((b|t|u|s)(\d+)\))");
        std::smatch match;
        std::string::const_iterator searchStart(hlslCode.cbegin());

        while (std::regex_search(searchStart, hlslCode.cend(), match, resourceRegex)) 
        {
            ShaderResource res;
            res.type = match[1];  // Type (cbuffer, Texture2D, etc.)
            res.name = match[2];  // Name
            res.registerIndex = std::stoi(match[4]); // Register index
            res.visibility = mapShaderTypeToVisibility(type);
            resources.push_back(res);
            
            searchStart = match.suffix().first;
        }

        std::regex constantBufferRegex(R"(ConstantBuffer\s*<\s*(\w+)\s*>\s*(\w+)\s*:\s*register\(b(\d+)\)\s*;)");
        searchStart = hlslCode.cbegin();
        while (std::regex_search(searchStart, hlslCode.cend(), match, constantBufferRegex)) 
        {
            ShaderResource res;
            res.type = "ConstantBuffer";
            res.name = match[2];  // Variable name
            res.registerIndex = std::stoi(match[3]);  // No explicit register index, must be auto-assigned
            res.visibility = mapShaderTypeToVisibility(type);
            resources.push_back(res);

            searchStart = match.suffix().first;
        }
        std::regex structuredBufferRegex(R"(StructuredBuffer\s*<\s*(\w+)\s*>\s*(\w+)\s*:\s*register\(t(\d+)\)\s*;)");
        while (std::regex_search(searchStart, hlslCode.cend(), match, structuredBufferRegex))
        {
            ShaderResource res;
            res.type = "StructuredBuffer";
            res.name = match[2];  // Variable name
            res.registerIndex = std::stoi(match[3]); // Extract explicit register index
            res.visibility = mapShaderTypeToVisibility(type);
            resources.push_back(res);
            searchStart = match.suffix().first;
        }

        return resources;
    }


    ID3D12RootSignature* CreateRootSignature(std::vector<ShaderResource>& resources, std::unordered_map<NameHash, RootParameterIndex>& descriptorRangeMap)
    {
        auto device = Renderer::get_instance()->get_device();
        std::vector<CD3DX12_ROOT_PARAMETER> rootParameters;
        std::vector<CD3DX12_DESCRIPTOR_RANGE> descriptorRanges;
        std::vector< D3D12_STATIC_SAMPLER_DESC> samplers;


        // Check for resources with the same name and register in different shader stages.
        for (int i = 0; i < resources.size(); i++)
        {
            if (resources[i].isDuplicate) continue;
            for (int j = i + 1; j < resources.size(); j++)
            {
                if (resources[i].name == resources[j].name && resources[i].registerIndex == resources[j].registerIndex)
                {
                    resources[j].isDuplicate = true;
                    resources[i].visibility = D3D12_SHADER_VISIBILITY_ALL;
                }
                else if (resources[i].name == resources[j].name && resources[i].registerIndex != resources[j].registerIndex)
                {
                    std::cerr << "Resource with name " << resources[i].name << " has different register index in different shader stages." << std::endl;
                    return nullptr;
                }
            }
        }



        for (const auto& res : resources)
        {
            if (res.isDuplicate) continue;
            if (res.type == "cbuffer" || res.type == "ConstantBuffer")
            {
                CD3DX12_ROOT_PARAMETER param;
                param.InitAsConstantBufferView(res.registerIndex, 0, res.visibility);
                rootParameters.push_back(param);
            }
            else if (res.type == "StructuredBuffer")
            {
                CD3DX12_ROOT_PARAMETER param;
                param.InitAsShaderResourceView(res.registerIndex, 0, res.visibility);
                rootParameters.push_back(param);
            }
            else if (res.type == "Texture2D")
            {
                CD3DX12_DESCRIPTOR_RANGE range;
                range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, res.registerIndex, 0);

                CD3DX12_ROOT_PARAMETER param;
                param.InitAsDescriptorTable(1, &range, res.visibility);

                rootParameters.push_back(param);
            }
            else if (res.type == "RWTexture2D")
            {
                CD3DX12_ROOT_PARAMETER param;
                param.InitAsUnorderedAccessView(res.registerIndex, 0, res.visibility);
                rootParameters.push_back(param);
            }
            else if (res.type == "SamplerState")
            {
                // yeah we probably should just parse the settings from hlsl file, but for now we will settle for a placeholder
                CD3DX12_STATIC_SAMPLER_DESC desc;
                desc.RegisterSpace = 0;
                desc.ShaderRegister = res.registerIndex;
                desc.ShaderVisibility = res.visibility;
                desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                desc.MipLODBias = 0;
                desc.MaxAnisotropy = 0;
                desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
                desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
                samplers.push_back(desc);
            }

            NameHash hash = olej_utils::murmurHash(reinterpret_cast<const u8*>(res.name.data()), res.name.size(), 69);

            descriptorRangeMap[hash] = rootParameters.size() - 1;
        }


        
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
            static_cast<UINT>(rootParameters.size()), rootParameters.data(),
            samplers.size(), samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        );

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr))
        {
            std::cerr << "Failed to serialize root signature: " << (char*)error->GetBufferPointer() << std::endl;
            return nullptr;
        }

        ID3D12RootSignature* rootSignature;
        hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature));

        if (FAILED(hr))
        {
            std::cerr << "Failed to create root signature" << std::endl;
            return nullptr;
        }

        return rootSignature;
    }

    std::string ReadFileToString(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }


    ID3D12RootSignature* PSOParser::parseRootSignature(std::vector<Shader*> const& shaders, std::unordered_map<NameHash, RootParameterIndex>& descriptorRangeMap)
    {
        std::vector<ShaderResource> resources;
        int i = 0;
        for (const auto& shader : shaders)
        {
            std::string hlslCode = ReadFileToString(std::string(shader->getFullPath().begin(), shader->getFullPath().end()));
            std::vector<ShaderResource> shaderResources = ExtractResources(hlslCode, i, shader->getType());
            resources.insert(resources.end(), shaderResources.begin(), shaderResources.end());
            i++;
        }

        auto rootSignature = CreateRootSignature(resources, descriptorRangeMap);
        return rootSignature;
    }

    CD3DX12_RASTERIZER_DESC parseRasterizerSettings(Shader* pixelShader)
    {
        assert(pixelShader->getType() == ShaderType::PIXEL);

        std::ifstream file(pixelShader->getFullPath());
        if (!file) {
            std::cerr << "While parsing rasterizer settings, an error opening file: " << std::string(pixelShader->getFullPath().begin(), pixelShader->getFullPath().end()) << std::endl;
            return CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        }

        std::string line;
        bool inSettings = false;
        bool inRasterizer = false;
        std::regex settingPattern(R"((\w+)\s*=\s*([\w]+);)");
        CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
        while (std::getline(file, line))
        {
            line.erase(0, line.find_first_not_of(" \t")); // Trim leading spaces

            if (line == "#ifdef SETTINGS")
            {
                inSettings = true;
            }
            else if (line == "#endif")
            {
                inSettings = false;
                inRasterizer = false;
            }
            else if (inSettings)
            {
                if (line == "RasterizerDescriptor")
                {
                    inRasterizer = true;
                }
                else if (inRasterizer)
                {
                    std::smatch match;
                    if (std::regex_search(line, match, settingPattern))
                    {
                        std::string setting = match[1];
                        std::string value = match[2];
                        if (setting == "FillMode")
                        {
                            if (value == "SOLID")
                            {
                                rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
                            }
                            else if (value == "WIREFRAME")
                            {
                                rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
                            }
                        }
                        else if (setting == "CullMode")
                        {
                            if (value == "NONE")
                            {
                                rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
                            }
                            else if (value == "FRONT")
                            {
                                rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
                            }
                            else if (value == "BACK")
                            {
                                rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
                            }
                        }
                        else if (setting == "FrontCounterClockwise")
                        {
                            rasterizerDesc.FrontCounterClockwise = value == "TRUE";
                        }
                        else if (setting == "DepthBias")
                        {
                            rasterizerDesc.DepthBias = std::stoi(value);
                        }
                        else if (setting == "DepthBiasClamp")
                        {
                            rasterizerDesc.DepthBiasClamp = std::stof(value);
                        }
                        else if (setting == "SlopeScaledDepthBias")
                        {
                            rasterizerDesc.SlopeScaledDepthBias = std::stof(value);
                        }
                        else if (setting == "DepthClipEnable")
                        {
                            rasterizerDesc.DepthClipEnable = value == "TRUE";
                        }
                        else if (setting == "MultisampleEnable")
                        {
                            rasterizerDesc.MultisampleEnable = value == "TRUE";
                        }
                        else if (setting == "AntialiasedLineEnable")
                        {
                            rasterizerDesc.AntialiasedLineEnable = value == "TRUE";
                        }
                        else if (setting == "ForcedSampleCount")
                        {
                            rasterizerDesc.ForcedSampleCount = std::stoi(value);
                        }
                        else if (setting == "ConservativeRaster")
                        {
                            if (value == "FALSE")
                            {
                                rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
                            }
                            else if (value == "TRUE")
                            {
                                rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
                            }
                        }
                    }
                }
            }
        }

        return rasterizerDesc;
    }
}

