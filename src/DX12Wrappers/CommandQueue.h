/**
 * Wrapper class for a ID3D12CommandQueue.
 */

#pragma once

#include <d3d12.h>  // For ID3D12CommandQueue, ID3D12Device2, and ID3D12Fence
#include <wrl.h>    // For Microsoft::WRL::ComPtr

#include <cstdint>  // For uint64_t
#include <queue>    // For std::queue

class CommandQueue
{
public:
    CommandQueue(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type);
    virtual ~CommandQueue();

    // Get an available command list from the command queue.
    ID3D12GraphicsCommandList2* get_command_list();

    // Execute a command list.
    // Returns the fence value to wait for for this command list.
    uint64_t execute_command_list(ID3D12GraphicsCommandList2* commandList);

    uint64_t signal();
    bool is_fence_complete(uint64_t fenceValue);
    void wait_for_fence_value(uint64_t fenceValue);
    void flush();

    ID3D12CommandQueue* get_d_3d12_command_queue() const;
protected:

    ID3D12CommandAllocator* create_command_allocator();
    ID3D12GraphicsCommandList2* create_command_list(ID3D12CommandAllocator* allocator);

private:
    // Keep track of command allocators that are "in-flight"
    struct CommandAllocatorEntry
    {
        uint64_t fenceValue;
        ID3D12CommandAllocator* commandAllocator;
    };

    using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
    using CommandListQueue = std::queue< ID3D12GraphicsCommandList2* >;

    D3D12_COMMAND_LIST_TYPE                     m_CommandListType;
    ID3D12Device2* m_d3d12Device;
    ID3D12CommandQueue* m_d3d12CommandQueue = nullptr;
    ID3D12Fence* m_d3d12Fence;
    HANDLE                                      m_FenceEvent;
    uint64_t                                    m_FenceValue;

    CommandAllocatorQueue                       m_CommandAllocatorQueue;
    CommandListQueue                            m_CommandListQueue;
};