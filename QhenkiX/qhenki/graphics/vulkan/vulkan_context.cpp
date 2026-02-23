#include "vulkan_context.h"

using namespace qhenki::gfx;

std::string VulkanContext::create(bool enable_debug_layer)
{
    return "";
}

bool VulkanContext::is_compatibility() const
{
    return false;
}

bool VulkanContext::create_swapchain(const DisplayWindow& window,
                                     const SwapchainDesc& swapchain_desc,
                                     Swapchain* swapchain,
                                     Queue* direct_queue,
                                     unsigned* frame_index)
{
}

bool VulkanContext::resize_swapchain(
    Swapchain* swapchain, int width, int height, DescriptorHeap* rtv_heap, unsigned& frame_index)
{
}

bool VulkanContext::create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap* rtv_heap)
{
}

bool VulkanContext::present(Swapchain* swapchain, unsigned fence_count, Fence* wait_fences, unsigned swapchain_index)
{
}

unsigned VulkanContext::get_swapchain_frame_index(const Swapchain& swapchain)
{
}

bool VulkanContext::create_shader(void* data, size_t size, ShaderType type, Shader* shader)
{
}

bool VulkanContext::create_pipeline(const GraphicsPipelineDesc& desc,
                                    GraphicsPipeline* pipeline,
                                    const Shader& vertex_shader,
                                    const Shader& pixel_shader,
                                    PipelineLayout* in_layout,
                                    const char* debug_name)
{
}

bool VulkanContext::bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline)
{
}

bool VulkanContext::create_pipeline_layout(PipelineLayoutDesc* desc, PipelineLayout* layout)
{
}

void VulkanContext::bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout)
{
}

bool VulkanContext::set_pipeline_constant(
    CommandList* cmd_list, unsigned param, uint32_t offset, unsigned size, void* data)
{
}

bool VulkanContext::create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap* heap, const char* debug_name)
{
}

void VulkanContext::set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap)
{
}

void VulkanContext::set_descriptor_heap(CommandList* cmd_list,
                                        const DescriptorHeap& heap,
                                        const DescriptorHeap& sampler_heap)
{
}

void VulkanContext::set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor)
{
}

bool VulkanContext::copy_descriptors(unsigned count, const Descriptor& src, const Descriptor& dst)
{
}

bool VulkanContext::free_descriptor(Descriptor* descriptor)
{
}

bool VulkanContext::create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, const char* debug_name)
{
}

bool VulkanContext::create_descriptor_constant_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor)
{
}

bool VulkanContext::create_descriptor_shader_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor)
{
}

void VulkanContext::copy_buffer(
    CommandList* cmd_list, const Buffer& src, uint64_t src_offset, Buffer* dst, uint64_t dst_offset, uint64_t bytes)
{
}

bool VulkanContext::create_texture(const TextureDesc& desc, Texture* texture, const char* debug_name)
{
}

bool VulkanContext::create_descriptor_shader_view(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor)
{
    return false;
}

bool VulkanContext::create_descriptor_render_target(const Texture& texture,
                                                    DescriptorHeap* heap,
                                                    Descriptor* descriptor)
{
}

bool VulkanContext::create_descriptor_depth_stencil(const Texture& texture,
                                                    DescriptorHeap* heap,
                                                    Descriptor* descriptor)
{
}

bool VulkanContext::copy_to_texture(CommandList* cmd_list, const void* data, Buffer* staging, Texture* texture)
{
}

bool VulkanContext::create_descriptor(const SamplerDesc& desc, DescriptorHeap* heap, Descriptor* descriptor)
{
}

void* VulkanContext::map_buffer(const Buffer& buffer)
{
}

void VulkanContext::unmap_buffer(const Buffer& buffer)
{
}

void VulkanContext::bind_vertex_buffers(CommandList* cmd_list,
                                        unsigned start_slot,
                                        unsigned buffer_count,
                                        const Buffer* const* buffers,
                                        const unsigned* sizes,
                                        const unsigned* strides,
                                        const unsigned* offsets)
{
}

void VulkanContext::bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format, unsigned offset)
{
}

bool VulkanContext::create_queue(QueueType type, Queue* queue)
{
}

bool VulkanContext::create_command_pool(CommandPool* command_pool, const Queue& queue)
{
}

bool VulkanContext::create_command_list(CommandList* cmd_list, const CommandPool& command_pool, const char* debug_name)
{
}

bool VulkanContext::reset_command_list(CommandList* cmd_list, const CommandPool& command_pool)
{
}

bool VulkanContext::close_command_list(CommandList* cmd_list)
{
}

bool VulkanContext::reset_command_pool(CommandPool* command_pool)
{
}

bool VulkanContext::start_render_pass(CommandList* cmd_list,
                                      Swapchain* swapchain,
                                      const float* clear_color_values,
                                      const RenderTarget* depth_stencil,
                                      unsigned frame_index)
{
}

bool VulkanContext::start_render_pass(CommandList* cmd_list,
                                      unsigned rt_count,
                                      const RenderTarget* const* rts,
                                      const RenderTarget* depth_stencil)
{
}

void VulkanContext::set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport)
{
}

void VulkanContext::set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect)
{
}

void VulkanContext::draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset)
{
}

void VulkanContext::draw_indexed(CommandList* cmd_list,
                                 uint32_t index_count,
                                 uint32_t instance_count,
                                 uint32_t start_index_offset,
                                 int32_t base_vertex_offset,
                                 uint32_t instance_offset)
{
}

void VulkanContext::submit_command_lists(const SubmitInfo& submit_info, Queue* queue)
{
}

bool VulkanContext::create_fence(Fence* fence, uint64_t initial_value)
{
}

uint64_t VulkanContext::get_fence_value(const Fence& fence)
{
}

bool VulkanContext::wait_fences(const WaitInfo& info)
{
}

void VulkanContext::set_barrier_resource(unsigned count,
                                         ImageBarrier* barriers,
                                         const Swapchain& swapchain,
                                         unsigned frame_index)
{
}

void VulkanContext::set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target)
{
}

void VulkanContext::issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers)
{
}

void VulkanContext::init_imgui(const DisplayWindow& window, const Swapchain& swapchain)
{
}

void VulkanContext::start_imgui_frame()
{
}

void VulkanContext::render_imgui_draw_data(CommandList* cmd_list)
{
}

void VulkanContext::destroy_imgui()
{
}

bool VulkanContext::compatibility_set_constant_buffers(unsigned slot,
                                                       unsigned count,
                                                       Buffer* const* buffers,
                                                       PipelineStage stage)
{
    return false;
}

bool VulkanContext::compatibility_set_shader_buffers(unsigned slot,
                                                     unsigned count,
                                                     Descriptor* const* descriptors,
                                                     PipelineStage stage)
{
    return false;
}

bool VulkanContext::compatibility_set_uav_buffers(unsigned slot, unsigned count, Buffer* const* buffers)
{
    return false;
}

bool VulkanContext::compatibility_set_textures(
    unsigned slot, unsigned count, Descriptor* const* descriptors, AccessFlags flag, PipelineStage stage)
{
    return false;
}

bool VulkanContext::compatibility_set_samplers(unsigned slot,
                                               unsigned count,
                                               Descriptor* const* samplers,
                                               PipelineStage stage)
{
    return false;
}

bool VulkanContext::wait_idle(Queue* queue)
{
}

VulkanContext::~VulkanContext()
{
}
