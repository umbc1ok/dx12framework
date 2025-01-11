// Toy Engine @ 2024
// Author: Hubert Olejnik
#pragma once
#include <d3d12.h>
#include <string>
#include <vector>

#include "Editor.h"


struct ProfilerEntry
{
    std::string name;
	uint32_t start_index;
    uint32_t end_index;
	uint32_t nesting;
};

struct ResolvedProfilerEntry
{
    std::string name;
    float time;
    uint32_t nesting;
};

// This is bug-prone, as it assumes there is only one command list, change later.
class GPUProfiler
{
public:
	GPUProfiler() = default;
	~GPUProfiler() = default;

	static void create();
	static GPUProfiler* getInstance();

    ProfilerEntry* startEntry(ID3D12GraphicsCommandList6* cmdList, const std::string name);
    void endEntry(ID3D12GraphicsCommandList6* cmdList, ProfilerEntry* entry);

	void startFrame() { currentStampIndex = 0; }
	void startRecording();
	void endRecording(ID3D12GraphicsCommandList6* cmdList);
	float collectData();
	float frameTime() { return lastFrameTime; }
    int numEntries() { return entries.size(); }
	ResolvedProfilerEntry getEntryTime(uint32_t index) const;
	void unmap() {readbackBuffer->Unmap(0, nullptr); }

    void setDisplayMode(bool useMicroSeconds) { m_useMicroSeconds = useMicroSeconds; }
	bool useMicroSeconds() { return m_useMicroSeconds; }

    void drawEditor(EditorWindow* const& window);

private:
    static GPUProfiler* m_instance;


	uint32_t m_currentNesting = 0;
	uint64_t currentStampIndex = 0;
	float lastFrameTime = 0.0f;
	UINT64 m_gpuFrequency;

	UINT64* timestampData;

	ID3D12QueryHeap* queryHeap;
	ID3D12Resource* readbackBuffer;

	bool m_useMicroSeconds = false;

    std::vector<ProfilerEntry*> entries;
};

