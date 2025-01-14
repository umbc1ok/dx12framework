#include "GPUProfiler.h"

#include <imgui.h>
#include <Renderer.h>
#include <iostream>
#include <utils/ErrorHandler.h>

#include "Editor.h"
#include "MeshletBenchmark.h"

GPUProfiler* GPUProfiler::m_instance;

void GPUProfiler::create()
{
    m_instance = new GPUProfiler();
}

GPUProfiler* GPUProfiler::getInstance()
{
    return m_instance;
}

ProfilerEntry* GPUProfiler::startEntry(ID3D12GraphicsCommandList6* cmdList, const std::string name)
{
    cmdList->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, currentStampIndex);
    ProfilerEntry* entry = new ProfilerEntry();
    entry->name = name;
    entry->start_index = currentStampIndex;
    entry->end_index = NAN;
    entry->nesting = m_currentNesting;
    currentStampIndex++;
    m_currentNesting++;
    entries.push_back(entry);
    return entry;
}

void GPUProfiler::endEntry(ID3D12GraphicsCommandList6* cmdList, ProfilerEntry* entry)
{
    cmdList->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, currentStampIndex);
    entry->end_index = currentStampIndex;
    currentStampIndex++;
    m_currentNesting--;
}

void GPUProfiler::startRecording()
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
    bufferDesc.Width = sizeof(UINT64) * 1000; // Space for 1000 timestamps
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
    for (auto& entry : entries)
        delete entry;
    entries.clear();
}

float GPUProfiler::collectData()
{
    D3D12_RANGE readRange = { 0, sizeof(uint64_t) * currentStampIndex };
    readbackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&timestampData));
    
    Renderer::get_instance()->get_cmd_queue(D3D12_COMMAND_LIST_TYPE_DIRECT)->get_d_3d12_command_queue()->GetTimestampFrequency(&m_gpuFrequency);

    return -1.0f;
}

ResolvedProfilerEntry GPUProfiler::getEntryTime(uint32_t index) const
{
    double time = (static_cast<double>(timestampData[entries[index]->end_index]) - static_cast<double>(timestampData[entries[index]->start_index])) / m_gpuFrequency * 1000.0f * (m_useMicroSeconds ? 1000.0f : 1.0f);
    ResolvedProfilerEntry entry;
    entry.name = entries[index]->name;
    entry.time = static_cast<float>(time);
    entry.nesting = entries[index]->nesting;
    return entry;
}

void GPUProfiler::drawEditor(EditorWindow* const& window)
{
    bool is_still_open = true;
    auto profiler = GPUProfiler::getInstance();
    ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);
    profiler->collectData();
    bool useMicroSeconds = profiler->useMicroSeconds();
    if (ImGui::Checkbox("Use Microseconds", &useMicroSeconds))
    {
        profiler->setDisplayMode(useMicroSeconds);
    }

    for (int i = 0; i < profiler->numEntries(); i++)
    {
        auto resolvedEntry = profiler->getEntryTime(i);
        for (uint32_t j = 0; j < resolvedEntry.nesting; j++)
        {
            ImGui::Indent();
        }
        if (resolvedEntry.name.compare("Model Draw"))
        {
            MeshletBenchmark::getInstance()->update(resolvedEntry.time);
        }



        if (useMicroSeconds)
            ImGui::Text("* %s: %.3f us", resolvedEntry.name.c_str(), resolvedEntry.time);
        else
            ImGui::Text("* %s: %.3f ms", resolvedEntry.name.c_str(), resolvedEntry.time);

        for (uint32_t j = 0; j < resolvedEntry.nesting; j++)
        {
            ImGui::Unindent();
        }
    }



    profiler->unmap();
    ImGui::End();
}
