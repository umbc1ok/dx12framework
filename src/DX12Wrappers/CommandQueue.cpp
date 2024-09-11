#include "CommandQueue.h"

#include <iostream>

#include "utils/ErrorHandler.h"

CommandQueue::CommandQueue(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type)
    : m_FenceValue(0)
    , m_CommandListType(type)
    , m_d3d12Device(device)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    AssertFailed(m_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue)));
    AssertFailed(m_d3d12Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));

    m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(m_FenceEvent && "Failed to create fence event handle.");

}

CommandQueue::~CommandQueue()
{
    flush();
    m_d3d12CommandQueue->Release();
}

ID3D12GraphicsCommandList2* CommandQueue::get_command_list()
{
    ID3D12CommandAllocator* commandAllocator = nullptr;
    ID3D12GraphicsCommandList2* cmd_list = nullptr;

    if (!m_CommandAllocatorQueue.empty() && is_fence_complete(m_CommandAllocatorQueue.front().fenceValue))
    {
        commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
        m_CommandAllocatorQueue.pop();

        AssertFailed(commandAllocator->Reset());
    }
    else
    {
        commandAllocator = create_command_allocator();
    }

    if (!m_CommandListQueue.empty())
    {
        cmd_list = m_CommandListQueue.front();
        m_CommandListQueue.pop();

        AssertFailed(cmd_list->Reset(commandAllocator, nullptr));
    }
    else
    {
        cmd_list = create_command_list(commandAllocator);
    }

    // Associate the command allocator with the command list so that it can be
    // retrieved when the command list is executed.
    AssertFailed(cmd_list->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator));

    return cmd_list;
}

uint64_t CommandQueue::execute_command_list(ID3D12GraphicsCommandList2* commandList)
{
    commandList->Close();

    ID3D12CommandAllocator* commandAllocator;
    UINT dataSize = sizeof(commandAllocator);
    AssertFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

    ID3D12CommandList* const ppCommandLists[] = {
        commandList
    };

    m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
    uint64_t fenceValue = signal();

    m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
    m_CommandListQueue.push(commandList);

	// TODO: Do I need to release the command allocator?
    //commandAllocator->Release();
    return fenceValue;
}

uint64_t CommandQueue::signal()
{
    uint64_t fenceValue = ++m_FenceValue;
    AssertFailed(m_d3d12CommandQueue->Signal(m_d3d12Fence, fenceValue));

    return fenceValue;
}

bool CommandQueue::is_fence_complete(uint64_t fenceValue)
{
    return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::wait_for_fence_value(uint64_t fenceValue)
{
    if (is_fence_complete(fenceValue))
        return;

    m_d3d12Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
    ::WaitForSingleObject(m_FenceEvent, DWORD_MAX);
}

void CommandQueue::flush()
{
    wait_for_fence_value(signal());
}

ID3D12CommandQueue* CommandQueue::get_d_3d12_command_queue() const
{
    return m_d3d12CommandQueue;
}

ID3D12CommandAllocator* CommandQueue::create_command_allocator()
{
    ID3D12CommandAllocator* commandAllocator = nullptr;
    AssertFailed(m_d3d12Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)));
    return commandAllocator;
}

ID3D12GraphicsCommandList2* CommandQueue::create_command_list(ID3D12CommandAllocator* allocator)
{
    ID3D12GraphicsCommandList2* commandList;
    AssertFailed(m_d3d12Device->CreateCommandList(0, m_CommandListType, allocator, nullptr, IID_PPV_ARGS(&commandList)));

    return commandList;
}
