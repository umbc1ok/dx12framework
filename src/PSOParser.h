#pragma once
#include <d3dx12.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "DXMeshletGenerator/D3D12MeshletGenerator.h"

enum class ShaderType;
class Shader;

namespace PSOParser
{
    using NameHash = uint32_t;
    using RootParameterIndex = uint32_t;


    struct ShaderResource
    {
        std::string name;
        std::string type;
        int registerIndex;
        D3D12_SHADER_VISIBILITY visibility;
        bool isDuplicate = false;
    };


    ID3D12RootSignature* parseRootSignature(std::vector<Shader*> const&  shaders, std::unordered_map<NameHash, RootParameterIndex>& descriptorRangeMap);

}
