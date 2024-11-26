#include "GPUProfiler.h"
#include <Renderer.h>
#include <iostream>
#include <utils/ErrorHandler.h>

void GPUProfiler::insertTimeStamp(ID3D12GraphicsCommandList6* cmdList)
{
    cmdList->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, currentStampIndex);
    currentStampIndex++;
}

void GPUProfiler::startRecording(ID3D12GraphicsCommandList6* cmdList)
{
    auto device = Renderer::get_instance()->get_device();
    D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
    queryHeapDesc.Count = 1000; // At least two timestamps: one at the start and one at the end of the frame
    queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    queryHeapDesc.NodeMask = 0;

    AssertFailed(device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&queryHeap)));

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    // allocate memory for a 1000 timestamps
    bufferDesc.Width = sizeof(UINT64) * 1000; // Space for two timestamps
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;

    AssertFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&readbackBuffer)));


    insertTimeStamp(cmdList);
}

void GPUProfiler::endRecording(ID3D12GraphicsCommandList6* cmdList)
{

    cmdList->ResolveQueryData(
        queryHeap,
        D3D12_QUERY_TYPE_TIMESTAMP,
        0, // Start index
        currentStampIndex, // Number of queries
        readbackBuffer,
        0 // Destination buffer offset
    );

}

float GPUProfiler::collectData()
{
    UINT64* timestampData;
    D3D12_RANGE readRange = { 0, sizeof(UINT64) * 2 };
    readbackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&timestampData));

    UINT64 startTime = timestampData[0];
    UINT64 endTime = timestampData[1];
    readbackBuffer->Unmap(0, nullptr);


    UINT64 gpuFrequency;
    Renderer::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT)->get_d_3d12_command_queue()->GetTimestampFrequency(&gpuFrequency);

    double frameTimeInSeconds = static_cast<double>(endTime - startTime) / gpuFrequency;
    std::cout << frameTimeInSeconds << "\n";
    currentStampIndex = 0;
    lastFrameTime = frameTimeInSeconds * 1000.0f;
    return (float)frameTimeInSeconds * 1000.0f;
}
