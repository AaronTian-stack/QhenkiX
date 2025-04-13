#pragma once

#include <wrl/client.h>
#include "D3D12MemAlloc.h"

using Microsoft::WRL::ComPtr;

struct D3D12Texture
{
	//std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
	ComPtr<D3D12MA::Allocation> allocation;
};
