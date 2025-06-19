#pragma once
#include "smartpointer.h"

namespace qhenki::gfx
{
	struct DescriptorHeapDesc
	{
		enum class Type
		{
			CBV_SRV_UAV, // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
			SAMPLER,     // VK_DESCRIPTOR_TYPE_SAMPLER
			RTV,         // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
			DSV,         // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
		} type;
		enum class Visibility
		{
			CPU,
			GPU,
		} visibility; // RTV, DSV is CPU only
		unsigned descriptor_count;
	};

	struct DescriptorHeap
	{
		DescriptorHeapDesc desc;
		sPtr<void> internal_state;
	};
}