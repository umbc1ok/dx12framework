#include "TextureLoader.h"

#include <d3dx12.h>

#include "DDSTextureLoader.h"
#include <DirectXTex.h>

#include "DirectXHelpers.h"
#include "Renderer.h"
#include "utils/ErrorHandler.h"
#include "utils/Types.h"
#include "utils/Utils.h"
#include <codecvt>
#include <locale>

#include "WICTextureLoader.h"

Texture* TextureLoader::texture_from_file(std::string const& path)
{
	auto device = Renderer::get_instance()->get_device();
	auto cmdqueue = Renderer::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdlist = cmdqueue->get_command_list();
	HRESULT hr;

	
	// Informations about the texture resource
	DirectX::TexMetadata metadata;
	// Content of the texture resource
	DirectX::ScratchImage scratchImage;

    // Convert std::string to std::wstring
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wpath = converter.from_bytes(path);

    //hr = DirectX::LoadFromDDSFile(wpath.c_str(), DirectX::DDS_FLAGS_FORCE_RGB, &metadata, scratchImage);
	
    hr = DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, scratchImage);
	AssertFailed(hr);
	D3D12_RESOURCE_DESC texture_desc = {};
	texture_desc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		static_cast<u64>(metadata.width),
		static_cast<u32>(metadata.height),
		static_cast<u16>(metadata.arraySize));

	Texture* texture = new Texture();
    texture->path = path;

	CD3DX12_HEAP_PROPERTIES default_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	device->CreateCommittedResource(
		&default_heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&texture_desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture->resource));

	std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
	const DirectX::Image* pImages = scratchImage.GetImages();

	for (int i = 0; i < scratchImage.GetImageCount(); ++i)
	{
		auto& subresource = subresources[i];
		subresource.RowPitch = pImages[i].rowPitch;
		subresource.SlicePitch = pImages[i].slicePitch;
		subresource.pData = pImages[i].pixels;
	}

	u64 requiredSize = GetRequiredIntermediateSize(texture->resource, 0, subresources.size());

	ID3D12Resource* intermediate_resource;
	CD3DX12_HEAP_PROPERTIES upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);

	hr = device->CreateCommittedResource(
		&upload_heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&intermediate_resource));
	AssertFailed(hr);

	UpdateSubresources(cmdlist, texture->resource, intermediate_resource, 0, 0, subresources.size(), subresources.data());
	Renderer::transition_resource(cmdlist, texture->resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);


	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = metadata.format;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	// TODO: Handle more MipLevels
	srv_desc.Texture2D.MipLevels = 1;


    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&texture->heap));
	texture->SRV_CPU = texture->heap->GetCPUDescriptorHandleForHeapStart();
	texture->SRV_GPU = texture->heap->GetGPUDescriptorHandleForHeapStart();

	device->CreateShaderResourceView(texture->resource, &srv_desc, texture->SRV_CPU);
	auto fenceValue = cmdqueue->execute_command_list(cmdlist);
	cmdqueue->wait_for_fence_value(fenceValue);

	intermediate_resource->Release();
	scratchImage.Release();

	return texture;
}
