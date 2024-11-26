// Toy Engine @ 2024
// Author: Hubert Olejnik
#pragma once
#include <d3d12.h>

// This is bug-prone, as it assumes there is only one command list, change later.
class GPUProfiler
{
public:
	GPUProfiler() = default;
	~GPUProfiler() = default;

	void insertTimeStamp(ID3D12GraphicsCommandList6* cmdList);

	void startRecording(ID3D12GraphicsCommandList6* cmdList);
	void endRecording(ID3D12GraphicsCommandList6* cmdList);
	float collectData();
	float frameTime() { return lastFrameTime; }
private:

	long int currentStampIndex = 0;
	float lastFrameTime = 0.0f;
	ID3D12QueryHeap* queryHeap;
	ID3D12Resource* readbackBuffer;
};

