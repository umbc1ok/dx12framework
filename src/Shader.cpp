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
    case ShaderType::PIXEL:
        m_mainFunctionName = L"ps_main";
        m_shaderModel = L"ps_6_5";
        break;
    case ShaderType::MESH:
        m_mainFunctionName = L"ms_main";
        m_shaderModel = L"ms_6_5";
        break;
    case ShaderType::AMPLIFICATION:
        m_mainFunctionName = L"as_main";
        m_shaderModel = L"as_6_5";
        break;
    case ShaderType::VERTEX:
        m_mainFunctionName = L"vs_main";
        m_shaderModel = L"vs_6_5";
        break;
    }

    HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library));
    AssertFailed(hr);
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler));
    AssertFailed(hr);
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils));
    hr = m_utils->CreateDefaultIncludeHandler(&m_includeHandler);
    AssertFailed(hr);

    loadShader();
}

void Shader::loadShader()
{
    uint32_t codePage = DXC_CP_ACP;
    IDxcBlobEncoding* sourceBlob;
    HRESULT hr;
    hr = m_library->CreateBlobFromFile(m_path.c_str(), &codePage, &sourceBlob);
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
        m_utils->BuildArguments(m_path.c_str(), m_mainFunctionName.c_str(), m_shaderModel.c_str(), &arguments, 1, nullptr, 0, &preprocessArgs);

        IDxcResult* preprocessResult;
        AssertFailed(m_compiler->Compile(
            &sourceBuffer,
            preprocessArgs->GetArguments(), preprocessArgs->GetCount(),
            m_includeHandler.Get(),
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
        hash = std::to_wstring(olej_utils::murmurHash(static_cast<u8*>(preprocessedBlob->GetBufferPointer()), preprocessedBuffer->Size, 0));
        size_t size = 0;
        char* data = readShaderBlobFromFile(m_compiledPath + m_filename + hash + m_mainFunctionName, &size);
        
        IDxcBlobEncoding* encoded_blob;
        m_utils->CreateBlob(data, size, CP_UTF8, &encoded_blob);
        hr = encoded_blob->QueryInterface(IID_PPV_ARGS(&m_dxcBlob));
        if (data && SUCCEEDED(hr))
        {
            std::cout << "Shader succesfully reloaded" << std::endl;
            return;
        }
    }


    // Compile from preprocessed file
    {
        IDxcCompilerArgs* compileArguments;
        m_utils->BuildArguments(m_path.c_str(), m_mainFunctionName.c_str(), m_shaderModel.c_str(), nullptr, 0, nullptr, 0, &compileArguments);

        IDxcResult* compileResult;
        hr = m_compiler->Compile(
            preprocessedBuffer,
            compileArguments->GetArguments(), compileArguments->GetCount(),
            m_includeHandler.Get(),
            IID_PPV_ARGS(&compileResult));
        compileResult->GetResult(&m_dxcBlob);
        if (SUCCEEDED(hr))
        {
            compileResult->GetStatus(&hr);
        }
        if (compileResult)
        {
            IDxcBlobEncoding* errorsBlob;
            hr = compileResult->GetErrorBuffer(&errorsBlob);
            if (errorsBlob->GetBufferPointer() != nullptr)
            {
                wprintf(L"Compilation failed with errors:\n%hs\n",
                    (const char*)errorsBlob->GetBufferPointer());
            }
            std::cout << "Shader succesfully compiled" << std::endl;
        }
    }




    // Save the compiled shader to file
    saveCompiledShaderBlob(m_compiledPath + m_filename + hash + m_mainFunctionName, m_dxcBlob);
}

char* Shader::readShaderBlobFromFile(std::wstring const& path, size_t* p_size)
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



bool Shader::saveCompiledShaderBlob(std::wstring const& path, IDxcBlob* p_blob)
{
    size_t const last_slash_pos = path.find_last_of('/');
    std::wstring const filename = path.substr(last_slash_pos + 1);
    std::wstring const full_path = m_compiledPath + filename;

    std::filesystem::path const directory = m_compiledPath;

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
        std::cerr << "Failed to open file: " << olej_utils::wstringToString(full_path) << std::endl;
        return false;
    }

    file.write(static_cast<char const*>(p_blob->GetBufferPointer()), p_blob->GetBufferSize());

    if (!file)
    {
        std::cerr << "Failed to write to file: " << olej_utils::wstringToString(full_path) << std::endl;
        return false;
    }

    file.close();

    return true;
}

IDxcBlob* Shader::getBlob() const
{
    return m_dxcBlob;
}

