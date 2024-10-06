#include "Shader.h"

#include <d3dcommon.h>
#include <d3dcompiler.h>



#include <fstream>
#include <iostream>
#include <Windows.h>
#include <filesystem>

#include "Window.h"
#include "utils/ErrorHandler.h"
#include "utils/Types.h"
#include "utils/Utils.h"


Shader::Shader(std::string name, ShaderType type)
{
    m_type = type;
    m_path = "./res/shaders/" + name;
    switch (m_type)
    {
    case ShaderType::VERTEX:
        m_main_function_name = "vs_main";
        shader_model = "VS_6_0";
        break;
    case ShaderType::PIXEL:
        m_main_function_name = "ps_main";
        shader_model = "PS_6_0";
        break;
    }

    // TODO: Handle errors
    
    HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

    hr = library->CreateIncludeHandler(&include_handler);
    

    load_shader_dxc();
}

void Shader::load_shader()
{
    HRESULT hr;
    {
        ID3DBlob* shader_compile_errors_blob;

        size_t size = 0;
        char const* shader_source = read_hlsl_shader_from_file(m_path, &size);
        hr = D3DPreprocess(shader_source, size, m_path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, &shader_blob,
            &shader_compile_errors_blob);

        delete[] shader_source;

        if (FAILED(hr))
        {
            char const* error_string = nullptr;
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                error_string = "Error. Vertex shader file not found.";
            }
            else if (shader_compile_errors_blob)
            {
                error_string = static_cast<char const*>(shader_compile_errors_blob->GetBufferPointer());
                shader_compile_errors_blob->Release();
            }

            std::cout << error_string << "\n";
            return;
        }

        std::string const hash =
            std::to_string(olej_utils::murmur_hash(static_cast<u8*>(shader_blob->GetBufferPointer()), shader_blob->GetBufferSize(), 0));
        if (!read_file_to_blob(m_path + hash + m_main_function_name, &shader_blob))
        {
            hr = D3DCompile(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr, nullptr, nullptr, m_main_function_name.c_str(), shader_model.c_str(), 0, 0,
                &shader_blob, &shader_compile_errors_blob);

            if (FAILED(hr))
            {
                auto const error_string = static_cast<char const*>(shader_compile_errors_blob->GetBufferPointer());
                std::cout << error_string << "\n";
                return;
            }
            save_compiled_shader(m_path + hash + m_main_function_name, shader_blob);
        }
    }
}

void Shader::load_shader_dxc()
{
    uint32_t codePage = CP_UTF8;
    IDxcBlobEncoding* sourceBlob;
    HRESULT hr;
    if(m_type == ShaderType::MESH)
    {
        hr = library->CreateBlobFromFile(L"./res/shaders/mesh_pipeline/MS_BASIC.hlsl", &codePage, &sourceBlob);
    }
    else
    {
        hr = library->CreateBlobFromFile(L"./res/shaders/mesh_pipeline/PS_BASIC.hlsl", &codePage, &sourceBlob);
    }

    //if(FAILED(hr)) Handle file loading error...
    // TODO: Get rid of all hard-coded strings
    IDxcOperationResult* result;
    if (m_type == ShaderType::VERTEX)
    {
        hr = compiler->Compile(
            sourceBlob, // pSource
            L"./res/shaders/basic.hlsl", // pSourceName
            L"vs_main", // pEntryPoint
            L"vs_6_5", // pTargetProfile
            nullptr, 0, // pArguments, argCount
            nullptr, 0, // pDefines, defineCount
            include_handler.Get(), // pIncludeHandler
            &result); // ppResult
    }
    else if (m_type == ShaderType::MESH)
    {
        hr = compiler->Compile(
            sourceBlob, // pSource
            L"E:/repositories/mine/dx12framework/res/shaders/mesh_pipeline/MS_BASIC.hlsl", // pSourceName
            L"main", // pEntryPoint
            L"ms_6_5", // pTargetProfile
            nullptr, 0, // pArguments, argCount
            nullptr, 0, // pDefines, defineCount
            include_handler.Get(), // pIncludeHandler
            &result); // ppResult
    }
    else
    {
        hr = compiler->Compile(
            sourceBlob, // pSource
            L"E:/repositories/mine/dx12framework/res/shaders/mesh_pipeline/PS_BASIC.hlsl", // pSourceName
            L"main", // pEntryPoint
            L"ps_6_5", // pTargetProfile
            nullptr, 0, // pArguments, argCount
            nullptr, 0, // pDefines, defineCount
            include_handler.Get(), // pIncludeHandler
            &result); // ppResult
    }


    if (SUCCEEDED(hr))
    {
        result->GetStatus(&hr);
    }
    if (FAILED(hr))
    {
        if (result)
        {
            IDxcBlobEncoding* errorsBlob;
            hr = result->GetErrorBuffer(&errorsBlob);
            if (SUCCEEDED(hr) && errorsBlob)
            {
                wprintf(L"Compilation failed with errors:\n%hs\n",
                    (const char*)errorsBlob->GetBufferPointer());
            }
        }
        // Handle compilation error...
    }
    
    result->GetResult(&dxc_blob);

    //size_t size = 0;
    //char const* shader_source = read_hlsl_shader_from_file(m_path, &size);
    //const DxcBuffer* sourceBuffer = new DxcBuffer(shader_source, size, CP_UTF8);
    //IDxcBlobEncoding* sourceBlob;
    //uint32_t codePage = CP_UTF8;
    //library->CreateBlobFromFile(olej_utils::string_to_LPCWSTR(m_path), &codePage, &sourceBlob);
    //HRESULT hr;
    //LPCWSTR args = olej_utils::string_to_LPCWSTR("-P");
    //_GUID xd;
    //IDxcResult* result;
    //IDxcCompilerArgs* args = IDxcUtils::BuildArguments(m_path, "vs_main", "vs_6_0", nullptr, 0);
    //hr = compiler->Compile(sourceBuffer, &args, 1, include_handler.Get(), IID_PPV_ARGS(&result));

    //if (FAILED(hr))
    //{
    //    // TODO: Handle errors
    //    std::cout << "GOWNO0" << std::endl;
    //}
    //IDxcBlob* shaderBlob;
    //result->GetResult(&shaderBlob);
    //std::string const hash = std::to_string(olej_utils::murmur_hash(static_cast<u8*>(shaderBlob->GetBufferPointer()), shaderBlob->GetBufferSize(), 0));
    //if (!read_file_to_blob(m_path + hash + m_main_function_name, &shaderBlob));
    //{
    //    hr = compiler->Compile(sourceBuffer, nullptr, 0, include_handler.Get(), IID_PPV_ARGS(&result));

    //    if (FAILED(hr))
    //    {
    //        std::cout << "GOWNO" << "\n";
    //        return;
    //    }
    //    save_compiled_shader(m_path + hash + m_main_function_name, shader_blob);
    //}

}

char* Shader::read_hlsl_shader_from_file(std::string const& path, size_t* p_size)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        *p_size = 0;
        return nullptr;
    }

    std::streamsize const file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    auto const buffer = new char[file_size + 1];

    if (file.read(buffer, file_size))
    {
        buffer[file_size] = '\0'; // Null-terminate the string
        *p_size = static_cast<size_t>(file_size);
        return buffer;
    }

    delete[] buffer;
    *p_size = 0;
    return nullptr;
}

bool Shader::read_file_to_blob(std::string const& path, ID3DBlob** pp_blob)
{
    size_t const last_slash_pos = path.find_last_of('/');
    std::string const filename = path.substr(last_slash_pos + 1);
    std::string const full_path = m_compiled_path + filename;

    std::filesystem::path const directory = m_compiled_path;

    if (!std::filesystem::exists(directory))
    {
        std::filesystem::create_directories(directory);
    }

    if (!pp_blob)
    {
        return false;
    }

    std::ifstream file(full_path, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        return false;
    }

    std::streamsize const file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    HRESULT const hr = D3DCreateBlob(file_size, pp_blob);

    if (FAILED(hr))
    {
        std::cerr << "Failed to create blob." << std::endl;
        return false;
    }

    if (!file.read(static_cast<char*>((*pp_blob)->GetBufferPointer()), file_size))
    {
        std::cerr << "Failed to read file: " << full_path + "\n";
        (*pp_blob)->Release();
        *pp_blob = nullptr;
        return false;
    }

    return true;
}

bool Shader::save_compiled_shader(std::string const& path, ID3DBlob* p_blob)
{
    size_t const last_slash_pos = path.find_last_of('/');
    std::string const filename = path.substr(last_slash_pos + 1);
    std::string const full_path = m_compiled_path + filename;

    std::filesystem::path const directory = m_compiled_path;

    if (!std::filesystem::exists(directory))
    {
        std::filesystem::create_directories(directory);
    }

    if (!p_blob)
    {
        std::cerr << "Invalid ID3DBlob pointer." << std::endl;
        return false;
    }

    std::ofstream file(full_path, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << full_path << std::endl;
        return false;
    }

    file.write(static_cast<char const*>(p_blob->GetBufferPointer()), p_blob->GetBufferSize());

    if (!file)
    {
        std::cerr << "Failed to write to file: " << full_path << std::endl;
        return false;
    }

    file.close();

    return true;
}

ID3DBlob* Shader::get_blob()
{
    return shader_blob;
}
