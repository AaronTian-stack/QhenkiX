#pragma once

#include <directx/dxgiformat.h>
#include <vulkan/vulkan.h>

#include "qhenki/RHI/barrier.h"
#include "qhenki/RHI/enums.h"
#include "qhenki/RHI/sampler.h"
#include "qhenki/RHI/texture.h"

namespace qhenki::gfx
{
VkFormat convert_format(DXGI_FORMAT format);

VkFormat get_vk_index_format(IndexType format);
VkPrimitiveTopology get_primitive_topology(PrimitiveTopology topology);
VkSamplerAddressMode texture_address_mode(AddressMode mode);
VkCompareOp comparison_func(ComparisonFunc func);
VkBlendFactor blend_factor(Blend b);
VkImageViewType view_type_from_desc(const TextureDesc& desc);
VkFilter get_vk_filter(Filter f);
VkSamplerMipmapMode get_vk_sampler_mipmap_mode(Filter f);
bool is_depth_stencil_format(VkFormat format);
VkImageAspectFlags get_image_aspect_mask(VkFormat format);

VkPipelineStageFlags2 sync_stage(SyncStage stage);
VkAccessFlags2 access_flags(AccessFlags access);
VkImageLayout layout(Layout layout);
} // namespace qhenki::gfx
