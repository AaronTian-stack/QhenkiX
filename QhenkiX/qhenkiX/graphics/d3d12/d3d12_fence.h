#pragma once
#include <wrl/client.h>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;

struct D3D12Fence
{
	HANDLE event;
	ComPtr<ID3D12Fence> fence;
};
