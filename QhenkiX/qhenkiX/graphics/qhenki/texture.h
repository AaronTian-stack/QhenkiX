#pragma once
#include <cstdint>

namespace qhenki::gfx
{
	enum class TextureDimension : uint8_t
	{
		TEXTURE_1D, TEXTURE_2D, TEXTURE_3D
	};

	struct TextureDesc
	{
		uint64_t width = 0;
		uint32_t height = 0;
		uint16_t depth_or_array_size = 1;
		uint16_t mip_levels = 1;
		DXGI_FORMAT format; // TODO: replace type
		//uint16_t sample_count = 1;
		TextureDimension dimension;
		Layout initial_layout;
	};

	struct Texture
	{
		TextureDesc desc;
		sPtr<void> internal_state;
	};
}
