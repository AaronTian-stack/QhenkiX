#pragma once

#include <stdexcept>
#include <string>

#include <qhenki/display_window.h>
#include "shader.h"
#include "swapchain.h"

#include "barrier.h"
#include "buffer.h"
#include "command_list.h"
#include "command_pool.h"
#include "descriptor.h"
#include "descriptor_heap.h"
#include "pipeline.h"
#include "qhenki/memory/arena.h"
#include "queue.h"
#include "render_target.h"
#include "sampler.h"
#include "submission.h"
#include "texture.h"

namespace qhenki::gfx
{
// TODO: replace all D3D types with qhenki::gfx types
class Context
{
public:
    /**
     * Creates the context.
     * @param enable_debug_layer Whether to enable underlying API's debug layer.
     * @return A string containing the error message on failure. Empty string on success.
     */
    virtual std::string create(bool enable_debug_layer) = 0;
    virtual bool is_compatibility() const = 0;

    virtual bool create_swapchain(const DisplayWindow& window,
                                  const SwapchainDesc& swapchain_desc,
                                  unsigned* frame_index) = 0;
    virtual bool resize_swapchain(Swapchain* swapchain, int width, int height, unsigned& frame_index) = 0;
    virtual bool present(const Swapchain& swapchain,
                         unsigned fence_count,
                         Fence* wait_fences,
                         unsigned swapchain_index) = 0;
    virtual unsigned get_swapchain_frame_index(const Swapchain& swapchain) = 0;

    virtual bool create_shader(void* data, size_t size, ShaderType type, Shader* shader) = 0;
    virtual bool create_pipeline(const GraphicsPipelineDesc& desc,
                                 GraphicsPipeline* pipeline,
                                 const Shader& vertex_shader,
                                 const Shader& pixel_shader,
                                 PipelineLayout* in_layout,
                                 const char* debug_name = nullptr) = 0;
    virtual bool bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline) = 0;

    virtual bool create_pipeline_layout(PipelineLayoutDesc* desc, PipelineLayout* layout) = 0;
    virtual void bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout) = 0;

    virtual bool set_pipeline_constant(
        CommandList* cmd_list, unsigned param, uint32_t offset, unsigned size, void* data) = 0;

    virtual bool create_descriptor_heap(const DescriptorHeapDesc& desc,
                                        DescriptorHeap* heap,
                                        const char* debug_name = nullptr) = 0;
    virtual size_t get_descriptor_heap_max_size(DescriptorHeapDesc::Type type) const = 0;
    virtual void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap) = 0;
    virtual void set_descriptor_heap(CommandList* cmd_list,
                                     const DescriptorHeap& heap,
                                     const DescriptorHeap& sampler_heap) = 0;

    virtual void set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor) = 0;
    virtual bool copy_descriptors(size_t bytes, const Descriptor& src, const Descriptor& dst) = 0;
    virtual bool free_descriptor(Descriptor* descriptor) = 0;
    virtual size_t get_descriptor_size(Descriptor::Type type) const = 0;
    virtual size_t get_descriptor_alignment(Descriptor::Type type) const = 0;

    /**
     * Creates a buffer with specified description.
     * @param desc Description struct for buffer to be created. The size of the buffer may be bloated to meet alignment
     * requirements.
     * @param data Pointer to initial data to be copied to the buffer. Only usable for CPU visible buffers.
     * @param buffer Pointer to buffer to be created. If already initialized, the preexisting buffer will be released
     * and then created.
     * @param debug_name String for debug name if supported by the API.
     * @return True if the operation succeeded, false otherwise.
     */
    virtual bool create_buffer(const BufferDesc& desc,
                               const void* data,
                               Buffer* buffer,
                               const char* debug_name = nullptr) = 0;
    virtual bool create_descriptor_constant_view(const Buffer& buffer,
                                                 DescriptorHeap* heap,
                                                 Descriptor* descriptor) = 0;
    virtual bool create_descriptor_shader_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor) = 0;

    virtual void copy_buffer(CommandList* cmd_list,
                             const Buffer& src,
                             uint64_t src_offset,
                             Buffer* dst,
                             uint64_t dst_offset,
                             uint64_t bytes) = 0;

    virtual bool create_texture(const TextureDesc& desc, Texture* texture, const char* debug_name = nullptr) = 0;
    // TODO: add description
    virtual bool create_descriptor_shader_view(const Texture& texture,
                                               DescriptorHeap* heap,
                                               Descriptor* descriptor) = 0;

    /**
     * @brief Creates staging buffer with data pointer and copies it to the texture.
     *
     * @param cmd_list Pointer to the command list used to record the copy operation.
     * @param data Pointer to the data to be copied.
     * @param staging Pointer to the uninitialized staging buffer.
     * @param texture Pointer to the destination texture where the data will be copied.
     * @return True if the copy operation was successful, false otherwise.
     */
    virtual bool copy_to_texture(CommandList* cmd_list, const void* data, Buffer* staging, Texture* texture) = 0;

    virtual bool create_descriptor(const SamplerDesc& desc, DescriptorHeap* heap, Descriptor* descriptor) = 0;

    // Write only
    virtual void* map_buffer(const Buffer& buffer) = 0;
    virtual void unmap_buffer(const Buffer& buffer) = 0;

    virtual bool bind_vertex_buffers(CommandList* cmd_list,
                                     unsigned start_slot,
                                     unsigned buffer_count,
                                     const Buffer* const* buffers,
                                     const uint64_t* sizes,
                                     const uint64_t* strides,
                                     const uint64_t* offsets) = 0;
    virtual void bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format, uint64_t offset) = 0;

    virtual bool create_command_pool(CommandPool* command_pool, QueueType queue) = 0;

    /**
     * Creates a command list. Thread safe only if a separate command pool is used for each thread, as modifying
     * CommandPool internals is not thread safe.
     * @param cmd_list Command list to be created. If already initialized, the preexisting command list will be released
     * and then created.
     * @param command_pool Command pool to allocate command list from.
     * @param debug_name String for debug name if supported by the API.
     * @return True if the operation succeeded, false otherwise.
     */
    virtual bool create_command_list(CommandList* cmd_list,
                                     const CommandPool& command_pool,
                                     const char* debug_name = nullptr) = 0;
    virtual bool reset_command_list(CommandList* cmd_list, const CommandPool& command_pool) = 0;
    virtual bool close_command_list(CommandList* cmd_list) = 0;

    virtual bool reset_command_pool(CommandPool* command_pool) = 0;

    /**
     * Starts a render pass on swap chain render target with optional depth stencil and clear color values.
     * @param cmd_list Command list to record render pass commands on.
     * @param clear_color_values Clear color values for render targets. If null, render targets will not be cleared.
     * @param depth_stencil Depth stencil render target. If null, no depth stencil will be bound.
     * @param frame_index Which swap chain buffer to use as render target.
     * @return True if the operation succeeded, false otherwise.
     */
    virtual bool start_render_pass(CommandList* cmd_list,
                                   const float* clear_color_values,
                                   const RenderTarget* depth_stencil,
                                   unsigned frame_index) = 0;
    virtual bool start_render_pass(CommandList* cmd_list,
                                   unsigned int rt_count,
                                   const RenderTarget* rts,
                                   const RenderTarget* depth_stencil) = 0;
    virtual void end_render_pass(CommandList* cmd_list) = 0;

    virtual void set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport) = 0;
    virtual void set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect) = 0;

    virtual void draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) = 0;
    virtual void draw_indexed(CommandList* cmd_list,
                              uint32_t index_count,
                              uint32_t instance_count,
                              uint32_t start_index_offset,
                              int32_t base_vertex_offset,
                              uint32_t instance_offset) = 0;
    // TODO: draw indirect
    // TODO: draw indirect count

    /**
     * Submits command lists to the specified queue and signals/waits on fences as specified in submit_info. Not thread
     * safe and should only be called from main thread.
     * @param submit_info Struct containing command lists to be submitted, fences to be signaled and waited on, and
     * what queues to wait on for each waiting fence.
     * @param queue Queue to submit command lists to.
     * @return True if the operation succeeded, false otherwise.
     */
    virtual bool submit_command_lists(const SubmitInfo& submit_info, QueueType queue) = 0;

    virtual bool create_fence(Fence* fence, uint64_t initial_value) = 0;
    // If submission is pending value may be out of date
    virtual uint64_t get_fence_value(const Fence& fence) = 0;
    // Waits for fences on CPU
    virtual bool wait_fences(const WaitInfo& info) = 0;

    // Sets ImageBarrier resource to swapchain resource
    virtual void set_barrier_resource(unsigned count,
                                      ImageBarrier* barriers,
                                      const Swapchain& swapchain,
                                      unsigned frame_index) = 0;
    virtual void set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target) = 0;
    virtual void issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers) = 0;

    virtual void init_imgui(const DisplayWindow& window, const Swapchain& swapchain) = 0;
    virtual void start_imgui_frame() = 0;
    virtual void render_imgui_draw_data(CommandList* cmd_list) = 0;
    virtual void destroy_imgui() = 0;

    virtual bool compatibility_set_constant_buffers(unsigned slot,
                                                    unsigned count,
                                                    Buffer* const* buffers,
                                                    PipelineStage stage) = 0;
    virtual bool compatibility_set_shader_buffers(unsigned slot,
                                                  unsigned count,
                                                  Descriptor* const* descriptors,
                                                  PipelineStage stage) = 0;
    virtual bool compatibility_set_uav_buffers(unsigned slot, unsigned count, Buffer* const* buffers) = 0;
    virtual bool compatibility_set_textures(
        unsigned slot, unsigned count, Descriptor* const* descriptors, AccessFlags flag, PipelineStage stage) = 0;
    virtual bool compatibility_set_samplers(unsigned slot,
                                            unsigned count,
                                            Descriptor* const* samplers,
                                            PipelineStage stage) = 0;

    virtual bool wait_idle(QueueType queue) = 0;
    virtual ~Context() = default;

protected:
    unsigned m_frame_count = 0;
};

inline memory::Arena& acquire_arena(const uint64_t current_frame)
{
    thread_local memory::Arena arena(4 * util::MEGABYTE);
    thread_local uint64_t arena_frame = 0;
    if (arena_frame != current_frame)
    {
        arena_frame = current_frame;
        arena.reset();
    }
    return arena;
}

} // namespace qhenki::gfx

#define THROW_IF_FALSE(result)                                   \
    do                                                           \
    {                                                            \
        if (!result)                                             \
        {                                                        \
            throw std::runtime_error("Something went wrong!\n"); \
        }                                                        \
    } while (0)

#define THROW_IF_TRUE(result)                                    \
    do                                                           \
    {                                                            \
        if ((result))                                            \
        {                                                        \
            throw std::runtime_error("Something went wrong!\n"); \
        }                                                        \
    } while (0)
