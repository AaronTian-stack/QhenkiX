#pragma once

#define VK_NO_PROTOTYPES
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#include "qhenki/RHI/context.h"

namespace qhenki::gfx
{
class VulkanContext : public Context
{
    struct Capabilities
    {
        VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptor_heap_properties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT};
    } m_capabilities; // TODO

    vkb::Instance m_instance;
    vkb::Device m_device;
    vkb::Swapchain m_swapchain;
    VmaAllocator m_allocator = nullptr;

public:
    std::string create(bool enable_debug_layer) override;
    bool is_compatibility() const override;
    bool create_swapchain(const DisplayWindow& window,
                          const SwapchainDesc& swapchain_desc,
                          Queue* direct_queue,
                          unsigned* frame_index) override;
    bool resize_swapchain(
        Swapchain* swapchain, int width, int height, DescriptorHeap* rtv_heap, unsigned& frame_index) override;
    bool create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap* rtv_heap) override;
    bool present(const Swapchain& swapchain,
                 unsigned fence_count,
                 Fence* wait_fences,
                 unsigned swapchain_index) override;
    unsigned get_swapchain_frame_index(const Swapchain& swapchain) override;

    bool create_shader(void* data, size_t size, ShaderType type, Shader* shader) override;
    bool create_pipeline(const GraphicsPipelineDesc& desc,
                         GraphicsPipeline* pipeline,
                         const Shader& vertex_shader,
                         const Shader& pixel_shader,
                         PipelineLayout* in_layout,
                         const char* debug_name) override;

    bool bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline) override;

    bool create_pipeline_layout(PipelineLayoutDesc* desc, PipelineLayout* layout) override;
    void bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout) override;

    bool set_pipeline_constant(
        CommandList* cmd_list, unsigned param, uint32_t offset, unsigned size, void* data) override;
    bool create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap* heap, const char* debug_name) override;

    void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap) override;
    void set_descriptor_heap(CommandList* cmd_list,
                             const DescriptorHeap& heap,
                             const DescriptorHeap& sampler_heap) override;
    void set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor) override;

    bool copy_descriptors(unsigned count, const Descriptor& src, const Descriptor& dst) override;
    bool free_descriptor(Descriptor* descriptor) override;

    bool create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, const char* debug_name) override;
    bool create_descriptor_constant_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor) override;
    bool create_descriptor_shader_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor) override;

    void copy_buffer(CommandList* cmd_list,
                     const Buffer& src,
                     uint64_t src_offset,
                     Buffer* dst,
                     uint64_t dst_offset,
                     uint64_t bytes) override;

    bool create_texture(const TextureDesc& desc, Texture* texture, const char* debug_name) override;
    bool create_descriptor_shader_view(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor) override;
    bool create_descriptor_render_target(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor) override;
    bool create_descriptor_depth_stencil(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor) override;

    bool copy_to_texture(CommandList* cmd_list, const void* data, Buffer* staging, Texture* texture) override;

    bool create_descriptor(const SamplerDesc& desc, DescriptorHeap* heap, Descriptor* descriptor) override;

    void* map_buffer(const Buffer& buffer) override;
    void unmap_buffer(const Buffer& buffer) override;

    void bind_vertex_buffers(CommandList* cmd_list,
                             unsigned start_slot,
                             unsigned buffer_count,
                             const Buffer* const* buffers,
                             const unsigned* sizes,
                             const unsigned* strides,
                             const unsigned* offsets) override;

    void bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format, unsigned offset) override;

    bool create_queue(QueueType type, Queue* queue) override;
    bool create_command_pool(CommandPool* command_pool, const Queue& queue) override;
    bool create_command_list(CommandList* cmd_list, const CommandPool& command_pool, const char* debug_name) override;
    bool reset_command_list(CommandList* cmd_list, const CommandPool& command_pool) override;
    bool close_command_list(CommandList* cmd_list) override;

    bool reset_command_pool(CommandPool* command_pool) override;

    bool start_render_pass(CommandList* cmd_list,
                           Swapchain* swapchain,
                           const float* clear_color_values,
                           const RenderTarget* depth_stencil,
                           unsigned frame_index) override;
    bool start_render_pass(CommandList* cmd_list,
                           unsigned rt_count,
                           const RenderTarget* const* rts,
                           const RenderTarget* depth_stencil) override;

    void set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport) override;
    void set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect) override;

    void draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
    void draw_indexed(CommandList* cmd_list,
                      uint32_t index_count,
                      uint32_t instance_count,
                      uint32_t start_index_offset,
                      int32_t base_vertex_offset,
                      uint32_t instance_offset) override;

    void submit_command_lists(const SubmitInfo& submit_info, Queue* queue) override;

    bool create_fence(Fence* fence, uint64_t initial_value) override;
    uint64_t get_fence_value(const Fence& fence) override;
    bool wait_fences(const WaitInfo& info) override;

    void set_barrier_resource(unsigned count,
                              ImageBarrier* barriers,
                              const Swapchain& swapchain,
                              unsigned frame_index) override;
    void set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target) override;

    void issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers) override;

    void init_imgui(const DisplayWindow& window, const Swapchain& swapchain) override;
    void start_imgui_frame() override;
    void render_imgui_draw_data(CommandList* cmd_list) override;
    void destroy_imgui() override;

    // Vulkan does not implement compability functions
    bool compatibility_set_constant_buffers(unsigned slot,
                                            unsigned count,
                                            Buffer* const* buffers,
                                            PipelineStage stage) override;
    bool compatibility_set_shader_buffers(unsigned slot,
                                          unsigned count,
                                          Descriptor* const* descriptors,
                                          PipelineStage stage) override;
    bool compatibility_set_uav_buffers(unsigned slot, unsigned count, Buffer* const* buffers) override;
    bool compatibility_set_textures(
        unsigned slot, unsigned count, Descriptor* const* descriptors, AccessFlags flag, PipelineStage stage) override;
    bool compatibility_set_samplers(unsigned slot,
                                    unsigned count,
                                    Descriptor* const* samplers,
                                    PipelineStage stage) override;

    bool wait_idle(Queue* queue) override;

    VulkanContext() = default;
    ~VulkanContext() override;
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;
};
} // namespace qhenki::gfx
