#pragma once

#include <DirectXMath.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include "d3d11_layout_assembler.h"
#include "d3d11_multithread.h"

#include "qhenki/RHI/context.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace qhenki::gfx
{

class D3D11Context : public Context
{
    std::array<D3D11_VIEWPORT, 16> m_viewports;
    D3D11LayoutAssembler m_layout_assembler;

    ComPtr<IDXGIFactory6> m_dxgi_factory;
    ComPtr<ID3D11Debug> m_debug;
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_device_context;
    ComPtr<ID3D10Multithread> m_multithread;

    UINT m_frame_index = 0;
    bool m_allow_tearing = false;

public:
    std::string create(bool enable_debug_layer) override;
    bool is_compatibility() const override;
    bool create_swapchain(const DisplayWindow& window,
                          const SwapchainDesc& swapchain_desc,
                          Swapchain* swapchain,
                          Queue* direct_queue,
                          unsigned* frame_index) override;
    bool resize_swapchain(
        Swapchain* swapchain, int width, int height, DescriptorHeap* rtv_heap, unsigned& frame_index) override;
    bool create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap* rtv_heap) override;
    bool present(Swapchain* swapchain, unsigned fence_count, Fence* wait_fences, unsigned swapchain_index) override;
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
    // Heaps only store views in D3D11
    void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap) override;
    void set_descriptor_heap(CommandList* cmd_list,
                             const DescriptorHeap& heap,
                             const DescriptorHeap& sampler_heap) override;

    void set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor) override;
    bool copy_descriptors(unsigned count, const Descriptor& src, const Descriptor& dst) override;
    bool free_descriptor(Descriptor* descriptor) override;

    bool create_buffer(const BufferDesc& desc,
                       const void* data,
                       Buffer* buffer,
                       const char* debug_name = nullptr) override;
    bool create_descriptor_constant_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor) override;
    bool create_descriptor_shader_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor) override;

    void copy_buffer(CommandList* cmd_list,
                     const Buffer& src,
                     uint64_t src_offset,
                     Buffer* dst,
                     uint64_t dst_offset,
                     uint64_t bytes) override;

    bool create_texture(const TextureDesc& desc, Texture* texture, const char* debug_name = nullptr) override;
    bool create_descriptor_shader_view(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor) override;
    bool create_descriptor_depth_stencil(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor) override;

    bool copy_to_texture(CommandList* cmd_list, const void* data, Buffer* staging, Texture* texture) override;

    bool create_sampler(const SamplerDesc& desc, Sampler* sampler) override;
    bool create_descriptor(const Sampler& sampler, DescriptorHeap* heap, Descriptor* descriptor) override;

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

    bool create_command_list(CommandList* cmd_list,
                             const CommandPool& command_pool,
                             const char* debug_name = nullptr) override;
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
                      uint32_t start_index_offset,
                      int32_t base_vertex_offset) override;

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
                                    Sampler* const* samplers,
                                    PipelineStage stage) override;

    bool wait_idle(Queue* queue) override;

    D3D11Context() = default;
    ~D3D11Context() override;
    D3D11Context(const D3D11Context&) = delete;
    D3D11Context& operator=(const D3D11Context&) = delete;
    D3D11Context(D3D11Context&&) = delete;
    D3D11Context& operator=(D3D11Context&&) = delete;

    friend struct D3D11GraphicsPipeline;

private:
    D3D11MultithreadLock acquire_lock() const
    {
        return D3D11MultithreadLock(m_multithread.Get());
    }

    void enter_recording() const
    {
        if (m_multithread)
        {
            m_multithread->Enter();
        }
    }

    void leave_recording() const
    {
        if (m_multithread)
        {
            m_multithread->Leave();
        }
    }

    ID3D11DepthStencilView* start_dsv(const RenderTarget* depth_stencil) const;
};

// Will not work with things that do not derive from ID3D11DeviceChild
bool set_debug_name(ID3D11DeviceChild* obj, const char* debug_name);

} // namespace qhenki::gfx
