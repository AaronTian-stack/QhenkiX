#pragma once

#include <D3D12MemAlloc.h>
#include <directx/d3d12shader.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl/client.h>

#include <D3D12DescriptorHelpers/RenderTargetHelper.hpp>

#include "d3d12_descriptor_heap.h"
#include "qhenki/RHI/context.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
class D3D12Context : public Context
{
    struct Capabilities
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};     // TiledResourcesTier, ResourceBindingTier, ResourceHeapTier
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};   // SamplerFeedbackTier
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {}; // Enhanced barriers
        D3D12_FEATURE_DATA_D3D12_OPTIONS4 options4 = {};   // VariableShadingRateTier
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};   // RaytracingTier
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};   // MeshShaderTier
        D3D12_FEATURE_DATA_SHADER_MODEL shader_model = {}; // Shader Model
        bool allow_tearing;
    } m_capabilities;

    // TODO: Add limits (ie memory)

    ComPtr<IDXGIFactory6> m_dxgi_factory;

    ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> m_dred_settings;
    ComPtr<ID3D12Debug3> m_debug;
    ComPtr<IDXGIDebug1> m_dxgi_debug;

    ComPtr<ID3D12Device4> m_device;
    ComPtr<D3D12MA::Allocator> m_allocator;

    ComPtr<IDXGISwapChain3> m_swapchain;
    std::array<ComPtr<ID3D12Resource>, 2> m_swapchain_buffers; // 2 is upper limit

    ComPtr<IDxcUtils> m_library;

    D3D12DescriptorHeap m_imgui_heap{};              // ImGUI only
    std::array<Descriptor, 2> m_imgui_descriptors{}; // ImGUI only

    ComPtr<ID3D12CommandQueue> m_graphics_queue;
    ComPtr<ID3D12CommandQueue> m_compute_queue;
    ComPtr<ID3D12CommandQueue> m_copy_queue;

    Fence m_fence_wait_all{}; // For stalling queues
    uint64_t m_fence_wait_all_last_signaled = 0;

public:
    std::string create(bool enable_debug_layer) override;
    bool is_compatibility() const override;
    bool create_swapchain(const DisplayWindow& window,
                          const SwapchainDesc& swapchain_desc,
                          unsigned* frame_index) override;
    bool resize_swapchain(Swapchain* swapchain, int width, int height, unsigned& frame_index) override;
    bool present(const Swapchain& swapchain,
                 unsigned fence_count,
                 Fence* wait_fences,
                 unsigned swapchain_index) override;
    unsigned get_swapchain_frame_index() override;

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

    bool create_descriptor_heap(const DescriptorHeapDesc& desc,
                                DescriptorHeap* heap,
                                const char* debug_name = nullptr) override;
    size_t get_descriptor_heap_max_size(DescriptorHeapDesc::Type type) const override;
    void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap) override;
    void set_descriptor_heap(CommandList* cmd_list,
                             const DescriptorHeap& heap,
                             const DescriptorHeap& sampler_heap) override;

    void set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor) override;
    bool copy_descriptors(size_t bytes, const Descriptor& src, const Descriptor& dst) override;
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
    bool reset_command_list(CommandList* cmd_list, const CommandPool& command_pool) override;
    bool create_command_list(CommandList* cmd_list, const CommandPool& command_pool, const char* debug_name) override;
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

    // D3D12 does not implement compability functions
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

    D3D12Context() = default;
    ~D3D12Context() override;
    D3D12Context(const D3D12Context&) = delete;
    D3D12Context& operator=(const D3D12Context&) = delete;
    D3D12Context(D3D12Context&&) = delete;
    D3D12Context& operator=(D3D12Context&&) = delete;

private:
    D3D12_INPUT_ELEMENT_DESC* shader_reflection(ID3D12ShaderReflection* shader_reflection,
                                                const D3D12_SHADER_DESC& shader_desc,
                                                bool increment_slot) const;
    ID3D12CommandQueue* get_command_queue(QueueType queue) const;
};
} // namespace qhenki::gfx
