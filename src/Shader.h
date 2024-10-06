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
    Shader(std::string name, ShaderType type);

    void load_shader();
    void load_shader_dxc();
    char* read_hlsl_shader_from_file(std::string const& path, size_t* p_size);
    bool read_file_to_blob(std::string const& path, ID3DBlob** pp_blob);
    bool save_compiled_shader(std::string const& path, ID3DBlob* p_blob);
    ID3DBlob* get_blob();
    IDxcBlob* dxc_blob;

private:
    std::string m_path = {};
    ShaderType m_type = ShaderType::NONE;
    std::string m_compiled_path = "./res/shaders/compiled/";
    std::string m_main_function_name = {};
    std::string shader_model = {};
    ID3DBlob* shader_blob;

    Microsoft::WRL::ComPtr<IDxcLibrary> library;
    Microsoft::WRL::ComPtr<IDxcCompiler> compiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> include_handler;
};

