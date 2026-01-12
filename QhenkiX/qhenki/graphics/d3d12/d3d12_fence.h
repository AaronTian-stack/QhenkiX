#pragma once

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct D3D12Fence
{
    HANDLE event = nullptr;
    ComPtr<ID3D12Fence> fence;
    ~D3D12Fence()
    {
        if (event)
        {
            CloseHandle(event);
        }
    }
};
