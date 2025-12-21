#pragma once

namespace qhenki::gfx
{
	struct CommandList
	{
		sPtr<void> internal_state;
	private:
		std::array<const DescriptorHeap*, 2> m_current_bound_heaps{};

		friend class D3D12Context;
	};
}
