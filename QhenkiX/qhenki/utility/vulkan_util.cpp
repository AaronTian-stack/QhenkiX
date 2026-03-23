#include "vulkan_util.h"

namespace qhenki::gfx
{

#define SYNC_STAGE_MAP(X)                                                                                     \
    X(SYNC_NONE, VK_PIPELINE_STAGE_2_NONE)                                                                    \
    X(SYNC_ALL, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT)                                                         \
    X(SYNC_DRAW, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR)                                                    \
    X(SYNC_INDEX_INPUT, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT)                                                  \
    X(SYNC_VERTEX_SHADING, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT)                                             \
    X(SYNC_PIXEL_SHADING, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT)                                            \
    X(SYNC_DEPTH_STENCIL, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT)                                        \
    X(SYNC_RENDER_TARGET, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)                                    \
    X(SYNC_COMPUTE_SHADING, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT)                                           \
    X(SYNC_RAYTRACING, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR)                                        \
    X(SYNC_COPY, VK_PIPELINE_STAGE_2_TRANSFER_BIT)                                                            \
    X(SYNC_RESOLVE, VK_PIPELINE_STAGE_2_RESOLVE_BIT)                                                          \
    X(SYNC_EXECUTE_INDIRECT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT)                                           \
    X(SYNC_ALL_SHADING, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT)                                                 \
    X(SYNC_NON_PIXEL_SHADING, VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT)                              \
    X(SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO,                                             \
      VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR)                                               \
    X(SYNC_CLEAR_UNORDERED_ACCESS_VIEW, VK_PIPELINE_STAGE_2_CLEAR_BIT)                                        \
    X(SYNC_VIDEO_DECODE, VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR)                                            \
    X(SYNC_VIDEO_PROCESS, VK_PIPELINE_STAGE_2_NONE) /* No Vulkan equivalent TODO: Check this */               \
    X(SYNC_VIDEO_ENCODE, VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR)                                            \
    X(SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE, VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR) \
    X(SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE, VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR)

#define ACCESS_FLAGS_MAP(X)                                                                                \
    X(ACCESS_COMMON, VK_ACCESS_2_NONE)                                                                     \
    X(ACCESS_VERTEX_BUFFER, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT)                                         \
    X(ACCESS_UNIFORM_BUFFER, VK_ACCESS_2_UNIFORM_READ_BIT)                                                 \
    X(ACCESS_INDEX_BUFFER, VK_ACCESS_2_INDEX_READ_BIT)                                                     \
    X(ACCESS_RENDER_TARGET, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)                                        \
    X(ACCESS_STORAGE_ACCESS, (VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT)) \
    X(ACCESS_DEPTH_STENCIL_WRITE, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)                          \
    X(ACCESS_DEPTH_STENCIL_READ, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT)                            \
    X(ACCESS_SHADER_RESOURCE, VK_ACCESS_2_SHADER_READ_BIT)                                                 \
    X(ACCESS_INDIRECT_ARGUMENT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT)                                     \
    X(ACCESS_COPY_DEST, VK_ACCESS_2_TRANSFER_WRITE_BIT)                                                    \
    X(ACCESS_COPY_SOURCE, VK_ACCESS_2_TRANSFER_READ_BIT)                                                   \
    X(ACCESS_RESOLVE_DEST, VK_ACCESS_2_TRANSFER_WRITE_BIT)  /* TODO: Check this */                         \
    X(ACCESS_RESOLVE_SOURCE, VK_ACCESS_2_TRANSFER_READ_BIT) /* TODO: Check this */                         \
    X(ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ, VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR)      \
    X(ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR)    \
    X(ACCESS_SHADING_RATE_SOURCE, VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR)               \
    X(ACCESS_VIDEO_DECODE_READ, VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR)                                     \
    X(ACCESS_VIDEO_DECODE_WRITE, VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR)                                   \
    X(ACCESS_VIDEO_PROCESS_READ, VK_ACCESS_2_NONE)                                                         \
    X(ACCESS_VIDEO_PROCESS_WRITE, VK_ACCESS_2_NONE)                                                        \
    X(ACCESS_VIDEO_ENCODE_READ, VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR)                                     \
    X(ACCESS_VIDEO_ENCODE_WRITE, VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR)                                   \
    X(NO_ACCESS, VK_ACCESS_2_NONE)

// GENERAL will work with everything on PC
// Prefer sync2 ATTACHMENT/READ_ONLY where appropriate
#define LAYOUT_MAP(X)                                                                    \
    X(UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED)                                              \
    X(COMMON, VK_IMAGE_LAYOUT_GENERAL)                                                   \
    X(PRESENT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)                                          \
    X(LAYOUT_GENERIC_READ, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR)                        \
    X(RENDER_TARGET, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR)                             \
    X(UNORDERED_ACCESS, VK_IMAGE_LAYOUT_GENERAL)                                         \
    X(DEPTH_STENCIL_WRITE, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR)                       \
    X(DEPTH_STENCIL_READ, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR)                         \
    X(SHADER_RESOURCE, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR)                            \
    X(COPY_SOURCE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)                                 \
    X(COPY_DEST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)                                   \
    X(RESOLVE_SOURCE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)                              \
    X(RESOLVE_DEST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)                                \
    X(SHADING_RATE_SOURCE, VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR) \
    X(VIDEO_DECODE_READ, VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR)                           \
    X(VIDEO_DECODE_WRITE, VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR)                          \
    X(VIDEO_PROCESS_READ, VK_IMAGE_LAYOUT_GENERAL)                                       \
    X(VIDEO_PROCESS_WRITE, VK_IMAGE_LAYOUT_GENERAL)                                      \
    X(VIDEO_ENCODE_READ, VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR)                           \
    X(VIDEO_ENCODE_WRITE, VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR)                          \
    X(DIRECT_QUEUE_COMMON, VK_IMAGE_LAYOUT_GENERAL)                                      \
    X(DIRECT_QUEUE_GENERIC_READ, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR)                  \
    X(DIRECT_QUEUE_UNORDERED_ACCESS, VK_IMAGE_LAYOUT_GENERAL)                            \
    X(DIRECT_QUEUE_SHADER_RESOURCE, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR)               \
    X(DIRECT_QUEUE_COPY_SOURCE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)                    \
    X(DIRECT_QUEUE_COPY_DEST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)                      \
    X(COMPUTE_QUEUE_COMMON, VK_IMAGE_LAYOUT_GENERAL)                                     \
    X(COMPUTE_QUEUE_GENERIC_READ, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR)                 \
    X(COMPUTE_QUEUE_UNORDERED_ACCESS, VK_IMAGE_LAYOUT_GENERAL)                           \
    X(COMPUTE_QUEUE_SHADER_RESOURCE, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR)              \
    X(COMPUTE_QUEUE_COPY_SOURCE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)                   \
    X(COMPUTE_QUEUE_COPY_DEST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)

#define DXGI_VK_FORMAT_MAP(X)                                            \
    X(DXGI_FORMAT_R32G32B32A32_FLOAT, VK_FORMAT_R32G32B32A32_SFLOAT)     \
    X(DXGI_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_UINT)        \
    X(DXGI_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_SINT)        \
    X(DXGI_FORMAT_R32G32B32_FLOAT, VK_FORMAT_R32G32B32_SFLOAT)           \
    X(DXGI_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32_UINT)              \
    X(DXGI_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32_SINT)              \
    X(DXGI_FORMAT_R16G16B16A16_FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT)     \
    X(DXGI_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_UNORM)      \
    X(DXGI_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT)        \
    X(DXGI_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R16G16B16A16_SNORM)      \
    X(DXGI_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT)        \
    X(DXGI_FORMAT_R32G32_FLOAT, VK_FORMAT_R32G32_SFLOAT)                 \
    X(DXGI_FORMAT_R32G32_UINT, VK_FORMAT_R32G32_UINT)                    \
    X(DXGI_FORMAT_R32G32_SINT, VK_FORMAT_R32G32_SINT)                    \
    X(DXGI_FORMAT_R10G10B10A2_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32) \
    X(DXGI_FORMAT_R10G10B10A2_UINT, VK_FORMAT_A2B10G10R10_UINT_PACK32)   \
    X(DXGI_FORMAT_R11G11B10_FLOAT, VK_FORMAT_B10G11R11_UFLOAT_PACK32)    \
    X(DXGI_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM)              \
    X(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, VK_FORMAT_R8G8B8A8_SRGB)          \
    X(DXGI_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT)                \
    X(DXGI_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM)              \
    X(DXGI_FORMAT_R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_SINT)                \
    X(DXGI_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM)              \
    X(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, VK_FORMAT_B8G8R8A8_SRGB)          \
    X(DXGI_FORMAT_B8G8R8X8_UNORM, VK_FORMAT_B8G8R8A8_UNORM)              \
    X(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB, VK_FORMAT_B8G8R8A8_SRGB)          \
    X(DXGI_FORMAT_R16G16_FLOAT, VK_FORMAT_R16G16_SFLOAT)                 \
    X(DXGI_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16_UNORM)                  \
    X(DXGI_FORMAT_R16G16_UINT, VK_FORMAT_R16G16_UINT)                    \
    X(DXGI_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16_SNORM)                  \
    X(DXGI_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SINT)                    \
    X(DXGI_FORMAT_R32_FLOAT, VK_FORMAT_R32_SFLOAT)                       \
    X(DXGI_FORMAT_R32_UINT, VK_FORMAT_R32_UINT)                          \
    X(DXGI_FORMAT_R32_SINT, VK_FORMAT_R32_SINT)                          \
    X(DXGI_FORMAT_D32_FLOAT, VK_FORMAT_D32_SFLOAT)                       \
    X(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT)    \
    X(DXGI_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT)        \
    X(DXGI_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM)                        \
    X(DXGI_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8_UNORM)                      \
    X(DXGI_FORMAT_R8G8_UINT, VK_FORMAT_R8G8_UINT)                        \
    X(DXGI_FORMAT_R8G8_SNORM, VK_FORMAT_R8G8_SNORM)                      \
    X(DXGI_FORMAT_R8G8_SINT, VK_FORMAT_R8G8_SINT)                        \
    X(DXGI_FORMAT_R16_FLOAT, VK_FORMAT_R16_SFLOAT)                       \
    X(DXGI_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM)                        \
    X(DXGI_FORMAT_R16_UINT, VK_FORMAT_R16_UINT)                          \
    X(DXGI_FORMAT_R16_SNORM, VK_FORMAT_R16_SNORM)                        \
    X(DXGI_FORMAT_R16_SINT, VK_FORMAT_R16_SINT)                          \
    X(DXGI_FORMAT_R8_UNORM, VK_FORMAT_R8_UNORM)                          \
    X(DXGI_FORMAT_R8_UINT, VK_FORMAT_R8_UINT)                            \
    X(DXGI_FORMAT_R8_SNORM, VK_FORMAT_R8_SNORM)                          \
    X(DXGI_FORMAT_R8_SINT, VK_FORMAT_R8_SINT)                            \
    X(DXGI_FORMAT_A8_UNORM, VK_FORMAT_R8_UNORM)                          \
    X(DXGI_FORMAT_BC1_UNORM, VK_FORMAT_BC1_RGBA_UNORM_BLOCK)             \
    X(DXGI_FORMAT_BC1_UNORM_SRGB, VK_FORMAT_BC1_RGBA_SRGB_BLOCK)         \
    X(DXGI_FORMAT_BC2_UNORM, VK_FORMAT_BC2_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC2_UNORM_SRGB, VK_FORMAT_BC2_SRGB_BLOCK)              \
    X(DXGI_FORMAT_BC3_UNORM, VK_FORMAT_BC3_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC3_UNORM_SRGB, VK_FORMAT_BC3_SRGB_BLOCK)              \
    X(DXGI_FORMAT_BC4_UNORM, VK_FORMAT_BC4_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC4_SNORM, VK_FORMAT_BC4_SNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC5_UNORM, VK_FORMAT_BC5_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC5_SNORM, VK_FORMAT_BC5_SNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC6H_UF16, VK_FORMAT_BC6H_UFLOAT_BLOCK)                \
    X(DXGI_FORMAT_BC6H_SF16, VK_FORMAT_BC6H_SFLOAT_BLOCK)                \
    X(DXGI_FORMAT_BC7_UNORM, VK_FORMAT_BC7_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC7_UNORM_SRGB, VK_FORMAT_BC7_SRGB_BLOCK)              \
    X(DXGI_FORMAT_B5G6R5_UNORM, VK_FORMAT_R5G6B5_UNORM_PACK16)           \
    X(DXGI_FORMAT_B5G5R5A1_UNORM, VK_FORMAT_A1R5G5B5_UNORM_PACK16)       \
    X(DXGI_FORMAT_B4G4R4A4_UNORM, VK_FORMAT_B4G4R4A4_UNORM_PACK16)       \
    X(DXGI_FORMAT_R9G9B9E5_SHAREDEXP, VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)

#define PRIMITIVE_TOPOLOGY_MAP(X)                         \
    X(POINT_LIST, VK_PRIMITIVE_TOPOLOGY_POINT_LIST)       \
    X(LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_LIST)         \
    X(LINE_STRIP, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP)       \
    X(TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) \
    X(TRIANGLE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)

#define ADDRESS_MODE_MAP(X)                            \
    X(WRAP, VK_SAMPLER_ADDRESS_MODE_REPEAT)            \
    X(MIRROR, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT) \
    X(CLAMP, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)    \
    X(BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)

#define COMPARISON_FUNC_MAP(X)                                \
    X(NONE, VK_COMPARE_OP_NEVER) /* No comparison sampling */ \
    X(NEVER, VK_COMPARE_OP_NEVER)                             \
    X(LESS, VK_COMPARE_OP_LESS)                               \
    X(EQUAL, VK_COMPARE_OP_EQUAL)                             \
    X(LESS_OR_EQUAL, VK_COMPARE_OP_LESS_OR_EQUAL)             \
    X(GREATER, VK_COMPARE_OP_GREATER)                         \
    X(NOT_EQUAL, VK_COMPARE_OP_NOT_EQUAL)                     \
    X(GREATER_OR_EQUAL, VK_COMPARE_OP_GREATER_OR_EQUAL)       \
    X(ALWAYS, VK_COMPARE_OP_ALWAYS)

VkFormat convert_format(const DXGI_FORMAT format)
{
#define MAP_DXGI_TO_VK(dxgi, vk) \
    case dxgi:                   \
        return vk;

    switch (format)
    {
        DXGI_VK_FORMAT_MAP(MAP_DXGI_TO_VK)
    default:
        assert(false);
        return VK_FORMAT_UNDEFINED;
    }

#undef MAP_DXGI_TO_VK
}

VkFormat get_vk_index_format(const IndexType format)
{
    if (format == IndexType::UINT16)
    {
        return VK_FORMAT_R16_UINT;
    }
    return VK_FORMAT_R32_UINT;
}

VkPrimitiveTopology get_primitive_topology(const PrimitiveTopology topology)
{
#define MAP_TOPOLOGY(our, vk)    \
    case PrimitiveTopology::our: \
        return vk;

    switch (topology)
    {
        PRIMITIVE_TOPOLOGY_MAP(MAP_TOPOLOGY)
    }

#undef MAP_TOPOLOGY
    assert(false);
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

VkSamplerAddressMode texture_address_mode(const AddressMode mode)
{
#define MAP_ADDRESS(our, vk) \
    case AddressMode::our:   \
        return vk;

    switch (mode)
    {
        ADDRESS_MODE_MAP(MAP_ADDRESS)
    }
    assert(false);
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;

#undef MAP_ADDRESS
}

VkCompareOp comparison_func(const ComparisonFunc func)
{
#define MAP_COMPARISON(our, vk) \
    case ComparisonFunc::our:   \
        return vk;

    switch (func)
    {
        COMPARISON_FUNC_MAP(MAP_COMPARISON)
    }
    assert(false);
    return VK_COMPARE_OP_NEVER;

#undef MAP_COMPARISON
}

VkImageViewType view_type_from_desc(const TextureDesc& desc)
{
    const uint32_t array_layers = desc.depth_or_array_size;
    switch (desc.dimension)
    {
    case TextureDimension::TEXTURE_1D:
        return array_layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
    case TextureDimension::TEXTURE_2D:
        if (desc.is_cube)
        {
            return array_layers > 6 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        }
        return array_layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    case TextureDimension::TEXTURE_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    }
    return VK_IMAGE_VIEW_TYPE_2D;
}

VkFilter get_vk_filter(const Filter f)
{
    return f == Filter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
}

VkSamplerMipmapMode get_vk_sampler_mipmap_mode(const Filter f)
{
    return f == Filter::LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
}

bool is_depth_stencil_format(const VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}

VkImageAspectFlags get_image_aspect_mask(const VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

VkPipelineStageFlags2 sync_stage(const SyncStage stage)
{
#define MAP_SYNC(our, vk) \
    if (stage & (our))    \
        flags |= (vk);

    VkPipelineStageFlags2 flags = 0;
    SYNC_STAGE_MAP(MAP_SYNC)
    return flags;

#undef MAP_SYNC
}

VkAccessFlags2 access_flags(const AccessFlags access)
{
#define MAP_ACCESS(our, vk) \
    if (access & (our))     \
        flags |= (vk);

    VkAccessFlags2 flags = 0;
    ACCESS_FLAGS_MAP(MAP_ACCESS)
    return flags;

#undef MAP_ACCESS
}

VkImageLayout layout(const Layout layout)
{
#define MAP_LAYOUT(our, vk) \
    case Layout::our:       \
        return vk;

    switch (layout)
    {
        LAYOUT_MAP(MAP_LAYOUT)
    }

#undef MAP_LAYOUT
    assert(false);
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

} // namespace qhenki::gfx
#undef SYNC_STAGE_MAP
#undef ACCESS_FLAGS_MAP
#undef LAYOUT_MAP
#undef DXGI_VK_FORMAT_MAP
#undef PRIMITIVE_TOPOLOGY_MAP
#undef ADDRESS_MODE_MAP
#undef COMPARISON_FUNC_MAP
