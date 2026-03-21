#pragma once

#define VK_NO_PROTOTYPES
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include "concurrentqueue.h"
#include "qhenki/graphics/shared/descriptor_flush.h"
#include "qhenki/RHI/context.h"

#define VK_SUCCEEDED(result) ((result) == VK_SUCCESS)
#define VK_FAILED(result) ((result) != VK_SUCCESS)

namespace qhenki::gfx
{
struct VulkanTexture;

class VulkanContext : public Context
{
    struct Capabilities
    {
        VkPhysicalDeviceProperties2 properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                                               .pNext = &descriptor_heap_properties};
        VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptor_heap_properties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT};
    } m_capabilities; // TODO

    vkb::Instance m_instance;
    vkb::Device m_device;
    struct
    {
        vkb::Swapchain swapchain;
        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
    } m_swapchain;
    VkSurfaceKHR m_surface;
    std::array<VkSemaphore, 2> m_image_available_semaphores{VK_NULL_HANDLE, VK_NULL_HANDLE};
    std::array<VkSemaphore, 2> m_render_finished_semaphores{VK_NULL_HANDLE, VK_NULL_HANDLE};

    VmaAllocator m_allocator = nullptr;

    struct VulkanQueue
    {
        VkQueue queue = VK_NULL_HANDLE;
        unsigned family_index = 0u;
        VkSemaphore semaphore = VK_NULL_HANDLE;
        uint64_t last_signaled_fence_value = 0;
    };

    VulkanQueue m_graphics_queue;
    VulkanQueue m_compute_queue;
    VulkanQueue m_transfer_queue;

    moodycamel::ConcurrentQueue<Texture*> m_texture_queue;
    VkSemaphore m_texture_submit_semaphore = VK_NULL_HANDLE;
    uint64_t m_texture_submit_fence_value = 0;
    VkCommandPool m_texture_transition_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_texture_transition_cmd_buffers;

    DeferredDescriptorCopier m_descriptor_copier;

public:
    std::string create(bool enable_debug_layer) override;
    bool is_compatibility() const override;
    bool create_swapchain(const DisplayWindow& window,
                          const SwapchainDesc& swapchain_desc,
                          unsigned* frame_index) override;
    bool resize_swapchain(Swapchain* swapchain, int width, int height, unsigned& frame_index) override;
    bool acquire_swapchain_image(unsigned* swapchain_index) override;
    bool present(const Swapchain& swapchain, unsigned swapchain_index) override;
    unsigned get_frame_slot(unsigned slot_count) const override;

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
    size_t get_descriptor_heap_max_size(DescriptorHeapDesc::Type type) const override;

    void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap) override;
    void set_descriptor_heap(CommandList* cmd_list,
                             const DescriptorHeap& heap,
                             const DescriptorHeap& sampler_heap) override;
    void set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor) override;

    bool copy_descriptors(size_t bytes, const Descriptor& src, const Descriptor& dst) override;
    bool flush_descriptor_copies(CommandList* cmd_list) override;

    bool free_descriptor(Descriptor* descriptor) override;
    size_t get_descriptor_size(Descriptor::Type type) const override;
    size_t get_descriptor_alignment(Descriptor::Type type) const override;

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

    bool copy_to_texture(CommandList* cmd_list, const void* data, Buffer* staging, Texture* texture) override;

    bool create_descriptor(const SamplerDesc& desc, DescriptorHeap* heap, Descriptor* descriptor) override;

    void* map_buffer(const Buffer& buffer) override;
    void unmap_buffer(const Buffer& buffer) override;

    bool bind_vertex_buffers(CommandList* cmd_list,
                             unsigned start_slot,
                             unsigned buffer_count,
                             const Buffer* const* buffers,
                             const uint64_t* sizes,
                             const uint64_t* strides,
                             const uint64_t* offsets) override;

    void bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format, uint64_t offset) override;

    bool create_command_pool(CommandPool* command_pool, QueueType queue) override;
    bool create_command_list(CommandList* cmd_list, const CommandPool& command_pool, const char* debug_name) override;
    bool reset_command_list(CommandList* cmd_list, const CommandPool& command_pool) override;
    bool close_command_list(CommandList* cmd_list) override;

    bool reset_command_pool(CommandPool* command_pool) override;

    bool start_render_pass(CommandList* cmd_list,
                           const float* clear_color_values,
                           const RenderTarget* depth_stencil,
                           unsigned frame_index) override;
    bool start_render_pass(CommandList* cmd_list,
                           unsigned rt_count,
                           const RenderTarget* rts,
                           const RenderTarget* depth_stencil) override;
    void end_render_pass(CommandList* cmd_list) override;

    void set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport) override;
    void set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect) override;

    void draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
    void draw_indexed(CommandList* cmd_list,
                      uint32_t index_count,
                      uint32_t instance_count,
                      uint32_t start_index_offset,
                      int32_t base_vertex_offset,
                      uint32_t instance_offset) override;

    bool submit_command_lists(const SubmitInfo& submit_info, QueueType queue) override;

    bool create_fence(Fence* fence, uint64_t initial_value) override;
    uint64_t get_fence_value(const Fence& fence) override;
    bool wait_fences(const WaitInfo& info) override;

    void set_barrier_resource(unsigned count,
                              ImageBarrier* barriers,
                              const Swapchain& swapchain,
                              unsigned frame_index) override;
    void set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target) override;

    bool issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers) override;

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

    bool wait_idle(QueueType queue) override;

    VulkanContext() = default;
    ~VulkanContext() override;
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    friend class VulkanDescriptorHeap;

private:
    // Grab an offset from for the descriptor in the heap
    bool allocate_descriptor(DescriptorHeap* heap,
                             const VulkanDescriptorHeap* vk_heap,
                             DescriptorHeapDesc::Type expected_heap_type,
                             Descriptor* descriptor,
                             Descriptor::Type descriptor_type) const;
    bool create_descriptor_buffer(const Buffer& buffer,
                                  DescriptorHeap* heap,
                                  Descriptor* descriptor,
                                  VkDescriptorType type) const;
    bool create_descriptor_texture(const Texture& texture,
                                   DescriptorHeap* heap,
                                   Descriptor* descriptor,
                                   VkDescriptorType type) const;

    VulkanQueue& get_queue(QueueType queue);
};
} // namespace qhenki::gfx
