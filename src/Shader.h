#pragma once
#include <d3dcommon.h>
#include <string>
#include <dxcapi.h>

#include <wrl/client.h>

//#pragma comment(lib, "dxcompiler.lib")

enum class ShaderType
{
    VERTEX,
    PIXEL,
    MESH,
    AMPLIFICATION,
    NONE
};

class Shader
{
public:
    Shader(std::wstring name, ShaderType type);

    void loadShader();
    char* readShaderBlobFromFile(std::wstring const& path, size_t* p_size);
    bool saveCompiledShaderBlob(std::wstring const& path, IDxcBlob* p_blob);
    IDxcBlob* getBlob() const;
    std::wstring const& getFullPath() const;
    ShaderType getType() const;

private:
    std::wstring m_path = {};
    ShaderType m_type = ShaderType::NONE;
    std::wstring m_compiledPath = L"./res/shaders/compiled/";
    std::wstring m_mainFunctionName = {};
    std::wstring m_shaderModel = {};
    std::wstring m_filename = {};
    IDxcBlob* m_dxcBlob;

    Microsoft::WRL::ComPtr<IDxcLibrary> m_library;
    Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_includeHandler;
    Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
};

