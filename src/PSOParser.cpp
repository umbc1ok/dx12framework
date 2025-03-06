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
            else if (res.type == "Texture2D" || res.type == "StructuredBuffer")
            {
                CD3DX12_ROOT_PARAMETER param;
                param.InitAsShaderResourceView(res.registerIndex, 0, res.visibility);
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
                CD3DX12_ROOT_PARAMETER param;
                param.InitAsDescriptorTable(1, &descriptorRanges.back(), res.visibility);
                rootParameters.push_back(param);
            }

            NameHash hash = olej_utils::murmurHash(reinterpret_cast<const u8*>(res.name.data()), res.name.size(), 69);

            descriptorRangeMap[hash] = rootParameters.size() - 1;
        }



        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
            static_cast<UINT>(rootParameters.size()), rootParameters.data(),
            0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
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

