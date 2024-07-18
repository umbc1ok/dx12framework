#pragma once
#include <d3dcommon.h>
#include <string>

enum class ShaderType
{
    VERTEX,
    PIXEL,
    NONE
};

class Shader
{
public:
    Shader(std::string name, ShaderType type);

    void load_shader();
    char* read_hlsl_shader_from_file(std::string const& path, size_t* p_size);
    bool read_file_to_blob(std::string const& path, ID3DBlob** pp_blob);
    bool save_compiled_shader(std::string const& path, ID3DBlob* p_blob);

private:
    std::string m_path = {};
    ShaderType m_type = ShaderType::NONE;
    std::string m_compiled_path = "./res/shaders/compiled/";
    std::string m_main_function_name = {};
    std::string shader_model = {};
    ID3DBlob* shader_blob;
};

