#pragma once

namespace qhenki::gfx
{
	enum class Filter
	{
		NEAREST,
		LINEAR,
	};
	enum class AddressMode
	{
		WRAP,
		MIRROR,
		CLAMP,
		BORDER,
	};
	enum class ComparisonFunc
	{
		NONE,
		NEVER,
		LESS,
		EQUAL,
		LESS_OR_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_OR_EQUAL,
		ALWAYS,
	};
	struct SamplerDesc
	{
		unsigned max_anisotropy = 0; // 0 to disable
		float min_lod = 0;
		float max_lod = D3D12_FLOAT32_MAX;
		float mip_lod_bias = 0;
		float border_color[4]{ 0.f, 0.f, 0.f, 0.f };
		Filter min_filter = Filter::LINEAR;
		Filter mag_filter = Filter::LINEAR;
		Filter mip_filter = Filter::LINEAR;
		AddressMode address_mode_u = AddressMode::WRAP;
		AddressMode address_mode_v = AddressMode::WRAP;
		AddressMode address_mode_w = AddressMode::WRAP;
		ComparisonFunc comparison_func = ComparisonFunc::NONE;
	};

	struct Sampler
	{
		SamplerDesc desc;
		sPtr<void> internal_state;
	};
}