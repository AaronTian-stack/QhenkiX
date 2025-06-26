#pragma once

#include <smartpointer.h>
#include "qhenkiX/helper/math_helper.h"

namespace qhenki::gfx
{
	enum class BufferUsage : uint8_t
	{
		VERTEX		= BIT(0),
		INDEX		= BIT(1),
		CONSTANT	= BIT(2),
		SHADER		= BIT(3), // UAV or Structured Buffer=
		UAV			= BIT(4), // UAV or Structured Buffer=
		INDIRECT	= BIT(5),
		COPY_SRC	= BIT(6),
		COPY_DST	= BIT(7),
	};

	constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) {
		using Underlying = std::underlying_type_t<BufferUsage>;
		return static_cast<BufferUsage>(
			static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs)
			);
	}

	constexpr bool operator&(BufferUsage lhs, BufferUsage rhs) {
		using Underlying = std::underlying_type_t<BufferUsage>;
		return (static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs)) != 0;
	}

	enum BufferVisibility : uint8_t
	{
		// Device local memory. Can be & with other visibilities to try to get BAR memory
		GPU				= BIT(0),
		// Host Visible: Written to by the CPU sequentially
		CPU_SEQUENTIAL	= BIT(1),
		// Host Visible: Written to by the CPU randomly. Cached randomly, try to avoid using this
		CPU_RANDOM		= BIT(2),
	};

    // Constant/Uniform buffers must follow D3D11 alignment rules (equivalent to std140 GLSL)
	struct BufferDesc
	{
		uint64_t size = 0;
		uint64_t stride = 0;
		BufferUsage usage; // Only used in D3D11 / Vulkan
		BufferVisibility visibility;
	};

	struct Buffer
	{
		BufferDesc desc;
		sPtr<void> internal_state;
	};
}
