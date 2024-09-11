#pragma once
#include <d3d12.h>

enum class TextureType
{
    None,
    Diffuse,
    Specular,
    Heightmap,
};

enum class TextureWrapMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class TextureFiltering
{
    None,
    Nearest,
    Linear,
};

struct Texture
{
    ID3D12Resource* resource;
    ID3D12DescriptorHeap* heap;
    D3D12_CPU_DESCRIPTOR_HANDLE SRV_CPU;
    D3D12_GPU_DESCRIPTOR_HANDLE SRV_GPU;
    TextureType type = TextureType::None;

    ~Texture()
    {
        resource->Release();
        heap->Release();
    }
};

