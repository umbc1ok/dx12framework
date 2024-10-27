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
    NONE
};

class Shader
{
public:
    Shader(std::wstring name, ShaderType type);

    void load_shader();
    void load_shader_dxc();
    char* read_hlsl_shader_from_file(std::wstring const& path, size_t* p_size);
    bool read_file_to_blob(std::wstring const& path, ID3DBlob** pp_blob);
    bool save_compiled_shader(std::wstring const& path, ID3DBlob* p_blob);
    ID3DBlob* get_blob();
    IDxcBlob* dxc_blob;

private:
    std::wstring m_path = {};
    ShaderType m_type = ShaderType::NONE;
    std::wstring m_compiled_path = L"./res/shaders/compiled/";
    std::wstring m_main_function_name = {};
    std::wstring shader_model = {};
    std::wstring m_filename = {};
    ID3DBlob* shader_blob;

    Microsoft::WRL::ComPtr<IDxcLibrary> library;
    Microsoft::WRL::ComPtr<IDxcCompiler> compiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> include_handler;
};

