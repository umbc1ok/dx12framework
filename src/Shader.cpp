#include "Shader.h"

#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxcapi.h>


#include <fstream>
#include <iostream>
#include <Windows.h>
#include <filesystem>

#include "Window.h"
#include "utils/ErrorHandler.h"
#include "utils/Types.h"
#include "utils/Utils.h"


Shader::Shader(std::wstring name, ShaderType type)
{
    m_type = type;
    m_path = L"./res/shaders/" + name;
    m_filename = name;
    switch (m_type)
    {
    case ShaderType::VERTEX:
        m_main_function_name = L"vs_main";
        shader_model = L"vs_6_5";
        break;
    case ShaderType::PIXEL:
        m_main_function_name = L"ps_main";
        shader_model = L"ps_6_5";
        break;
    case ShaderType::MESH:
        m_main_function_name = L"ms_main";
        shader_model = L"ms_6_5";
        break;
    }

    HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    AssertFailed(hr);
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    AssertFailed(hr);
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    hr = utils->CreateDefaultIncludeHandler(&include_handler);
    AssertFailed(hr);

    load_shader_dxc();
}

//void Shader::load_shader()
//{
//    HRESULT hr;
//    {
//        ID3DBlob* shader_compile_errors_blob;
//
//        size_t size = 0;
//        char const* shader_source = read_hlsl_shader_from_file(m_path, &size);
//        hr = D3DPreprocess(shader_source, size, olej_utils::wstring_to_string(m_path).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, &shader_blob,
//            &shader_compile_errors_blob);
//
//        delete[] shader_source;
//
//        if (FAILED(hr))
//        {
//            char const* error_string = nullptr;
//            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
//            {
//                error_string = "Error. Vertex shader file not found.";
//            }
//            else if (shader_compile_errors_blob)
//            {
//                error_string = static_cast<char const*>(shader_compile_errors_blob->GetBufferPointer());
//                shader_compile_errors_blob->Release();
//            }
//
//            std::cout << error_string << "\n";
//            return;
//        }
//
//        std::wstring const hash =
//            std::to_wstring(olej_utils::murmur_hash(static_cast<u8*>(shader_blob->GetBufferPointer()), shader_blob->GetBufferSize(), 0));
//        if (!read_file_to_blob(m_path + hash + m_main_function_name, &shader_blob))
//        {
//            hr = D3DCompile(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr, nullptr, nullptr, olej_utils::wstring_to_string(m_main_function_name).c_str(), olej_utils::wstring_to_string(shader_model).c_str(), 0, 0,
//                &shader_blob, &shader_compile_errors_blob);
//
//            if (FAILED(hr))
//            {
//                auto const error_string = static_cast<char const*>(shader_compile_errors_blob->GetBufferPointer());
//                std::cout << error_string << "\n";
//                return;
//            }
//            save_compiled_shader(m_path + hash + m_main_function_name, shader_blob);
//        }
//    }
//}

void Shader::load_shader_dxc()
{
    // TODO: Add shader reloading

    uint32_t codePage = DXC_CP_ACP;
    IDxcBlobEncoding* sourceBlob;
    HRESULT hr;
    hr = library->CreateBlobFromFile(m_path.c_str(), &codePage, &sourceBlob);
    AssertFailed(hr);

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
    sourceBuffer.Size = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_ACP; // Auto-detect the BOM

    // Define arguments for preprocessing and specify -P for preprocessed output
    const wchar_t* outputPreprocessedFile = L"preprocessed_shader.hlsl";


    IDxcBlob* preprocessedBlob;
    DxcBuffer* preprocessedBuffer = nullptr;
    // Preprocess the shader with `Compile` using `-P` option
    {
        IDxcCompilerArgs* preprocessArgs;
        LPCWSTR arguments = L"-P";
        utils->BuildArguments(m_path.c_str(), m_main_function_name.c_str(), shader_model.c_str(), &arguments, 1, nullptr, 0, &preprocessArgs);

        IDxcResult* preprocessResult;
        AssertFailed(compiler->Compile(
            &sourceBuffer,
            preprocessArgs->GetArguments(), preprocessArgs->GetCount(),
            include_handler.Get(),
            IID_PPV_ARGS(&preprocessResult)));
        preprocessResult->GetResult(&preprocessedBlob);
        preprocessedBuffer = new DxcBuffer();
        preprocessedBuffer->Ptr = preprocessedBlob->GetBufferPointer();
        preprocessedBuffer->Size = preprocessedBlob->GetBufferSize();
        preprocessedBuffer->Encoding = DXC_CP_ACP;
    }
    std::wstring hash;
    // Hash the preprocessed shader to check if it was already compiled before
    {
        hash = std::to_wstring(olej_utils::murmur_hash(static_cast<u8*>(preprocessedBlob->GetBufferPointer()), preprocessedBuffer->Size, 0));
        size_t size = 0;
        char* data = read_hlsl_shader_from_file(m_compiled_path + m_filename + hash + m_main_function_name, &size);
        
        IDxcBlobEncoding* encoded_blob;
        utils->CreateBlob(data, size, CP_UTF8, &encoded_blob);
        hr = encoded_blob->QueryInterface(IID_PPV_ARGS(&dxc_blob));
        if (data && SUCCEEDED(hr))
        {
            std::cout << "Shader succesfully reloaded" << std::endl;
            return;
        }
    }


    // Compile from preprocessed file
    {
        IDxcCompilerArgs* compileArguments;
        utils->BuildArguments(m_path.c_str(), m_main_function_name.c_str(), shader_model.c_str(), nullptr, 0, nullptr, 0, &compileArguments);

        IDxcResult* compileResult;
        hr = compiler->Compile(
            preprocessedBuffer,
            compileArguments->GetArguments(), compileArguments->GetCount(),
            include_handler.Get(),
            IID_PPV_ARGS(&compileResult));
        compileResult->GetResult(&dxc_blob);
        if (SUCCEEDED(hr))
        {
            compileResult->GetStatus(&hr);
        }
        if (compileResult)
        {
            IDxcBlobEncoding* errorsBlob;
            hr = compileResult->GetErrorBuffer(&errorsBlob);
            if (errorsBlob != nullptr)
            {
                wprintf(L"Compilation failed with errors:\n%hs\n",
                    (const char*)errorsBlob->GetBufferPointer());
            }
        }
    }




    // Save the compiled shader to file
    save_compiled_shader(m_compiled_path + m_filename + hash + m_main_function_name, dxc_blob);
}

char* Shader::read_hlsl_shader_from_file(std::wstring const& path, size_t* p_size)
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



bool Shader::save_compiled_shader(std::wstring const& path, IDxcBlob* p_blob)
{
    size_t const last_slash_pos = path.find_last_of('/');
    std::wstring const filename = path.substr(last_slash_pos + 1);
    std::wstring const full_path = m_compiled_path + filename;

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
        std::cerr << "Failed to open file: " << olej_utils::wstring_to_string(full_path) << std::endl;
        return false;
    }

    file.write(static_cast<char const*>(p_blob->GetBufferPointer()), p_blob->GetBufferSize());

    if (!file)
    {
        std::cerr << "Failed to write to file: " << olej_utils::wstring_to_string(full_path) << std::endl;
        return false;
    }

    file.close();

    return true;
}




ID3DBlob* Shader::get_blob()
{
    return shader_blob;
}

//bool Shader::read_file_to_blob(std::wstring const& path, IDxcBlob** pp_blob)
//{
//    size_t const last_slash_pos = path.find_last_of('/');
//    std::wstring const filename = path.substr(last_slash_pos + 1);
//    std::wstring const full_path = m_compiled_path + filename;
//
//    std::filesystem::path const directory = m_compiled_path;
//
//    if (!std::filesystem::exists(directory))
//    {
//        std::filesystem::create_directories(directory);
//    }
//
//    if (!pp_blob)
//    {
//        return false;
//    }
//
//    std::ifstream file(full_path, std::ios::binary | std::ios::ate);
//
//    if (!file.is_open())
//    {
//        return false;
//    }
//
//    std::streamsize const file_size = file.tellg();
//    file.seekg(0, std::ios::beg);
//    uint32_t codePage = CP_UTF8;
//    HRESULT const hr = library->CreateBlobFromFile(m_path.c_str(), &codePage, &sourceBlob);
//
//    if (FAILED(hr))
//    {
//        std::cerr << "Failed to create blob." << std::endl;
//        return false;
//    }
//
//    if (!file.read(static_cast<char*>((*pp_blob)->GetBufferPointer()), file_size))
//    {
//        std::cerr << "Failed to read file: " << olej_utils::wstring_to_string(full_path) + "\n";
//        (*pp_blob)->Release();
//        *pp_blob = nullptr;
//        return false;
//    }
//
//    return true;
//}