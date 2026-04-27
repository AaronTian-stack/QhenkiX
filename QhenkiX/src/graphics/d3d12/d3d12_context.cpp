#include "d3d12_context.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl3.h>

#include <DirectXTex.h>

#include <d3dcompiler.h>
#include <directx/d3d12shader.h>

#include <algorithm>
#include <boost/container/small_vector.hpp>

#include "d3d12_command_list.h"
#include "d3d12_pipeline.h"
#include "dxc_shader_compiler.h"
#include "qhenki/application.h"

#include "d3d12_descriptor_heap.h"
#include "d3d12_fence.h"
#include "d3d12_texture.h"

#include "qhenki/utility/gfx_util.h"
#include "qhenki/utility/string_util.h"
#include "src/utility/d3d_reflection_util.h"
#include "src/utility/d3d_util.h"

using namespace qhenki::gfx;

// Operator overloads for D3D12MA enums to work with designated initializers
namespace D3D12MA
{
constexpr ALLOCATOR_FLAGS operator|(const ALLOCATOR_FLAGS a, const ALLOCATOR_FLAGS b)
{
    return static_cast<ALLOCATOR_FLAGS>(static_cast<int>(a) | static_cast<int>(b));
}
} // namespace D3D12MA

namespace
{
D3D12DescriptorHeap* to_internal(const DescriptorHeap& ext)
{
    const auto d3d12_heap = static_cast<D3D12DescriptorHeap*>(ext.internal_state.get());
    assert(d3d12_heap);
    return d3d12_heap;
}

D3D12Pipeline* to_internal(const GraphicsPipeline& ext)
{
    const auto d3d12_pipeline = static_cast<D3D12Pipeline*>(ext.internal_state.get());
    assert(d3d12_pipeline);
    return d3d12_pipeline;
}

D3D12CommandList* to_internal(const CommandList& ext)
{
    const auto d3d12_cmd_list = static_cast<D3D12CommandList*>(ext.internal_state.get());
    assert(d3d12_cmd_list);
    return d3d12_cmd_list;
}

D3D12Fence* to_internal(const Fence& ext)
{
    const auto d3d12_fence = static_cast<D3D12Fence*>(ext.internal_state.get());
    assert(d3d12_fence);
    return d3d12_fence;
}

ComPtr<ID3D12CommandAllocator>* to_internal(const CommandPool& ext)
{
    const auto d3d12_cmd_pool = static_cast<ComPtr<ID3D12CommandAllocator>*>(ext.internal_state.get());
    assert(d3d12_cmd_pool);
    return d3d12_cmd_pool;
}

ComPtr<D3D12MA::Allocation>* to_internal(const Buffer& ext)
{
    const auto alloc = static_cast<ComPtr<D3D12MA::Allocation>*>(ext.internal_state.get());
    assert(alloc);
    return alloc;
}

ComPtr<ID3D12RootSignature>* to_internal(const PipelineLayout& ext)
{
    const auto root_sig = static_cast<ComPtr<ID3D12RootSignature>*>(ext.internal_state.get());
    assert(root_sig);
    return root_sig;
}

D3D12Texture* to_internal(const Texture& ext)
{
    const auto text = static_cast<D3D12Texture*>(ext.internal_state.get());
    assert(text);
    return text;
}

bool set_debug_name(ID3D12Object* obj, const char* name)
{
    if (obj && name)
    {
        return SUCCEEDED(obj->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name));
    }
    return false;
}
} // namespace

std::string D3D12Context::create(const bool enable_debug_layer)
{
    UINT dxgi_factory_flags = 0;
    if (enable_debug_layer)
    {
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug))))
        {
            m_debug->EnableDebugLayer();
            dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        }

        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_dred_settings))))
        {
            m_dred_settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            m_dred_settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        }
    }

    if (FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(m_dxgi_factory.ReleaseAndGetAddressOf()))))
    {
        return "D3D12: Failed to create DXGI factory";
    }

    BOOL allow_tearing = FALSE;
    if (FAILED(m_dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(BOOL))))
    {
        return "D3D12: Failed to check for allow tearing support";
    }
    m_capabilities.allow_tearing = allow_tearing == TRUE;

    if (enable_debug_layer)
    {
        m_dxgi_factory->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("DXGI Factory") - 1, "DXGI Factory");
    }

    // Pick discrete GPU
    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(m_dxgi_factory->EnumAdapterByGpuPreference(0, // Adapter index
                                                          DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                          __uuidof(IDXGIAdapter1),
                                                          reinterpret_cast<void**>(adapter.GetAddressOf()))))
    {
        return "D3D12: Failed to find a discrete GPU adapter";
    }

    DXGI_ADAPTER_DESC1 desc;
    auto hr = adapter->GetDesc1(&desc);
    if (SUCCEEDED(hr))
    {
        const auto msg = qhenki::util::format_wstring<256>(L"D3D12: Selected adapter: %ls\n", desc.Description);
        OutputDebugStringW(msg.buffer.data());
    }

    if (FAILED(
            D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()))))
    {
        return "D3D12: Failed to create device";
    }

    if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(m_library.ReleaseAndGetAddressOf()))))
    {
        return "D3D12: Failed to create DxcLibrary";
    }

    // Heap Tier is considered in D3D12MA library so technically no need to consider it here (yet)
    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS,
                                             &m_capabilities.options,
                                             sizeof(m_capabilities.options))))
    {
        return "D3D12: Failed to check feature support for D3D12_OPTIONS";
    }
    if (m_capabilities.options.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
    {
        return "D3D12: Resource Binding Tier 3 is required";
    }

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12,
                                             &m_capabilities.options12,
                                             sizeof(m_capabilities.options12))))
    {
        return "D3D12: Failed to check feature support for D3D12_OPTIONS12";
    }
    if (!m_capabilities.options12.EnhancedBarriersSupported)
    {
        return "D3D12: Enhanced barriers are not supported";
    }

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4,
                                             &m_capabilities.options4,
                                             sizeof(m_capabilities.options4))))
    {
        return "D3D12: Failed to check feature support for D3D12_OPTIONS4";
    }

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
                                             &m_capabilities.options5,
                                             sizeof(m_capabilities.options5))))
    {
        return "D3D12: Failed to check feature support for D3D12_OPTIONS5";
    }

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6,
                                             &m_capabilities.options6,
                                             sizeof(m_capabilities.options6))))
    {
        return "D3D12: Failed to check feature support for D3D12_OPTIONS5";
    }

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7,
                                             &m_capabilities.options7,
                                             sizeof(m_capabilities.options7))))
    {
        return "D3D12: Failed to check feature support for D3D12_OPTIONS5";
    }

    // Find the highest supported shader model
    constexpr std::array shader_models = {
        D3D_SHADER_MODEL_6_6,
        D3D_SHADER_MODEL_6_5,
        D3D_SHADER_MODEL_6_4,
        D3D_SHADER_MODEL_6_2,
        D3D_SHADER_MODEL_6_1,
        D3D_SHADER_MODEL_6_0,
    };

    for (const auto& model : shader_models)
    {
        m_capabilities.shader_model.HighestShaderModel = model;
        hr = m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,
                                           &m_capabilities.shader_model,
                                           sizeof(m_capabilities.shader_model));
        if (SUCCEEDED(hr))
        {
            break;
        }
    }
    if (m_capabilities.shader_model.HighestShaderModel < D3D_SHADER_MODEL_6_0)
    {
        return "D3D12: Shader Model >= SM_6_0 is required";
    }

    const D3D12MA::ALLOCATOR_DESC allocator_desc{
        .Flags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED,
        .pDevice = m_device.Get(),
        .pAdapter = adapter.Get(),
    };

    if (FAILED(CreateAllocator(&allocator_desc, &m_allocator)))
    {
        return "D3D12: Failed to create memory allocator";
    }

    if (enable_debug_layer && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_dxgi_debug))))
    {
        m_dxgi_debug->EnableLeakTrackingForThread();
    }

    auto create_queue = [&](const D3D12_COMMAND_LIST_TYPE type, ComPtr<ID3D12CommandQueue>& cmd_queue)
    {
        D3D12_COMMAND_QUEUE_DESC queue_desc{.Type = type};
        if (FAILED(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(cmd_queue.ReleaseAndGetAddressOf()))))
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create command queue\n");
            return false;
        }
        return true;
    };

    if (!create_queue(D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphics_queue))
    {
        return "D3D12: Failed to create graphics command queue";
    }

    if (!create_queue(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_compute_queue))
    {
        return "D3D12: Failed to create compute command queue";
    }

    if (!create_queue(D3D12_COMMAND_LIST_TYPE_COPY, m_copy_queue))
    {
        return "D3D12: Failed to create copy command queue";
    }

    if (!create_fence(&m_fence_wait_all, 0, "Internal stall fence"))
    {
        return "D3D12: Failed to create fence";
    }

    return "";
}

bool D3D12Context::is_compatibility() const
{
    return false;
}

bool D3D12Context::create_swapchain(const DisplayWindow& window, const SwapchainDesc& swapchain_desc)
{
    if (swapchain_desc.tearing && !m_capabilities.allow_tearing)
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Tearing is not supported on this system\n");
        return false;
    }

    UINT swap_chain_flags = 0;
    if (swapchain_desc.tearing && m_capabilities.allow_tearing)
    {
        swap_chain_flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    const DXGI_SWAP_CHAIN_DESC1 swap_chain_descriptor = {
        .Width = swapchain_desc.width,
        .Height = swapchain_desc.height,
        .Format = dxgi_format(swapchain_desc.format),
        .SampleDesc = {.Count = 1, // MSAA Count
                       .Quality = 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = swapchain_desc.buffer_count,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = swap_chain_flags,
    };

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_descriptor{};
    swap_chain_fullscreen_descriptor.Windowed = true;

    const auto hwnd = window.get_hwnd();
    ComPtr<IDXGISwapChain1> swapchain1;
    if (!hwnd || FAILED(m_dxgi_factory->CreateSwapChainForHwnd(m_graphics_queue.Get(), // Force flush on queue
                                                               hwnd,
                                                               &swap_chain_descriptor,
                                                               &swap_chain_fullscreen_descriptor,
                                                               nullptr,
                                                               swapchain1.ReleaseAndGetAddressOf())))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create Swapchain");
        return false;
    }
    ComPtr<IDXGISwapChain3> swapchain3;
    if (FAILED(swapchain1.As(&swapchain3)))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to get IDXGISwapChain3 from IDXGISwapChain1");
        return false;
    }

    m_swapchain.Reset();
    for (auto& buffer : m_swapchain_buffers)
    {
        buffer.Reset();
    }
    m_swapchain = swapchain3;

    for (unsigned i = 0; i < swapchain_desc.buffer_count; i++)
    {
        if (FAILED(m_swapchain->GetBuffer(i, IID_PPV_ARGS(m_swapchain_buffers[i].ReleaseAndGetAddressOf()))))
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to get back buffer from swap chain\n");
            m_swapchain.Reset();
            for (auto& buffer : m_swapchain_buffers)
            {
                buffer.Reset();
            }
            return false;
        }
    }

    return true;
}

bool D3D12Context::resize_swapchain(Swapchain* const swapchain, const unsigned width, const unsigned height)
{
    wait_idle(GRAPHICS);

    for (auto& buffer : m_swapchain_buffers)
    {
        buffer.Reset();
    }

    UINT resize_flags = 0;
    if (swapchain->tearing && m_capabilities.allow_tearing)
    {
        resize_flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    if (FAILED(m_swapchain->ResizeBuffers(
            swapchain->buffer_count, width, height, dxgi_format(swapchain->format), resize_flags)))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to resize swap chain buffers\n");
        return false;
    }

    for (unsigned i = 0; i < swapchain->buffer_count; i++)
    {
        if (FAILED(m_swapchain->GetBuffer(i, IID_PPV_ARGS(m_swapchain_buffers[i].ReleaseAndGetAddressOf()))))
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to get back buffer from swap chain\n");
            return false;
        }
    }

    swapchain->width = width;
    swapchain->height = height;
    m_swapchain_index = m_swapchain->GetCurrentBackBufferIndex();

    return true;
}

bool D3D12Context::present(const Swapchain& swapchain)
{
    ++m_frame_count;
    UINT sync_interval = 1;
    UINT flags = 0;
    if (swapchain.tearing && m_capabilities.allow_tearing)
    {
        sync_interval = 0;
        flags |= DXGI_PRESENT_ALLOW_TEARING;
    }
    if (FAILED(m_swapchain->Present(sync_interval, flags)))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to present");
        return false;
    }
    return true;
}

bool D3D12Context::acquire_swapchain_image()
{
    m_swapchain_index = m_swapchain->GetCurrentBackBufferIndex();
    return true;
}

unsigned D3D12Context::get_frame_slot(const unsigned slot_count) const
{
    return slot_count > 0 ? m_frame_count % slot_count : 0;
}

D3D12_INPUT_ELEMENT_DESC* D3D12Context::shader_reflection(ID3D12ShaderReflection* shader_reflection,
                                                          const D3D12_SHADER_DESC& shader_desc,
                                                          const bool increment_slot) const
{
    assert(shader_reflection);

    auto& arena = acquire_arena(m_frame_count);
    const auto input_element_desc = arena.alloc_array<D3D12_INPUT_ELEMENT_DESC>(shader_desc.InputParameters);
    {
        UINT slot = 0;
        for (UINT parameter_index = 0; parameter_index < shader_desc.InputParameters; parameter_index++)
        {
            D3D12_SIGNATURE_PARAMETER_DESC signature_parameter_desc{};
            const auto hr = shader_reflection->GetInputParameterDesc(parameter_index, &signature_parameter_desc);

            if (FAILED(hr))
            {
                return nullptr;
            }

            const auto format = qhenki::gfx::mask_to_format(signature_parameter_desc.Mask,
                                                            signature_parameter_desc.ComponentType);
            if (format == DXGI_FORMAT_UNKNOWN)
            {
                return nullptr;
            }

            input_element_desc[parameter_index] = D3D12_INPUT_ELEMENT_DESC{
                .SemanticName = signature_parameter_desc.SemanticName,
                .SemanticIndex = signature_parameter_desc.SemanticIndex,
                .Format = format,
                .InputSlot = increment_slot ? slot++ : 0u,
                .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0u, // TODO: Manual options for instancing
            };
        }
    }

    return input_element_desc;
}

bool D3D12Context::create_pipeline(const GraphicsPipelineDesc& desc,
                                   GraphicsPipeline* const pipeline,
                                   const Shader vertex_shader,
                                   const Shader pixel_shader,
                                   PipelineLayout* in_layout,
                                   const char* debug_name)
{
    if (desc.num_render_targets > MAX_RENDER_TARGETS)
    {
        return false;
    }
    assert(pipeline);
    pipeline->internal_state = mkS<D3D12Pipeline>();
    auto d3d12_pipeline = to_internal(*pipeline);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc{};
    ComPtr<ID3D12ShaderReflection> shader_reflection;
    D3D12_SHADER_DESC shader_desc{};
    D3D12_INPUT_ELEMENT_DESC* input_layout_desc = nullptr;

    const DxcBuffer vs_container_buffer = {.Ptr = vertex_shader.data, .Size = vertex_shader.size, .Encoding = 0};

    // SM >= 6.0
    {
        // Prefer reflecting from the RDAT part if available to avoid scanning the container
        void* rdat_ptr = nullptr;
        UINT32 rdat_size = 0;
        auto hr =
            m_library->GetDxilContainerPart(&vs_container_buffer, DXC_PART_REFLECTION_DATA, &rdat_ptr, &rdat_size);
        if (SUCCEEDED(hr) && rdat_ptr && rdat_size)
        {
            const DxcBuffer rdat_buffer = {.Ptr = rdat_ptr, .Size = rdat_size, .Encoding = 0};
            hr = m_library->CreateReflection(&rdat_buffer, IID_PPV_ARGS(&shader_reflection));
        }
        else
        {
            hr = m_library->CreateReflection(&vs_container_buffer, IID_PPV_ARGS(&shader_reflection));
        }
        if (FAILED(hr))
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to reflect vertex shader\n");
            pipeline->internal_state.reset();
            return false;
        }

        if (FAILED(shader_reflection->GetDesc(&shader_desc)))
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to reflect vertex shader\n");
            pipeline->internal_state.reset();
            return false;
        }

        input_layout_desc = this->shader_reflection(shader_reflection.Get(), shader_desc, desc.increment_slot);
        if (input_layout_desc == nullptr)
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to get input layout description\n");
            return false;
        }

        pso_desc.VS = {.pShaderBytecode = vertex_shader.data, .BytecodeLength = vertex_shader.size};
        pso_desc.PS = {.pShaderBytecode = pixel_shader.data, .BytecodeLength = pixel_shader.size};
    }

    pso_desc.InputLayout = {.pInputElementDescs = input_layout_desc, .NumElements = shader_desc.InputParameters};

    // Prefer root signature embedded in shader container if present
    ComPtr<ID3D12RootSignature> root_signature;
    {
        void* rsig_ptr = nullptr;
        UINT32 rsig_size = 0;
        if (SUCCEEDED(m_library->GetDxilContainerPart(
                &vs_container_buffer, DXC_PART_ROOT_SIGNATURE, &rsig_ptr, &rsig_size)) &&
            rsig_ptr && rsig_size)
        {
            assert(!in_layout); // Should not have both
            if (FAILED(m_device->CreateRootSignature(
                    0, rsig_ptr, rsig_size, IID_PPV_ARGS(root_signature.ReleaseAndGetAddressOf()))))
            {
                pipeline->internal_state.reset();
                return false;
            }
            pso_desc.pRootSignature = root_signature.Get();
            d3d12_pipeline->root_signature = root_signature;
        }
    }

    if (!pso_desc.pRootSignature)
    {
        const auto rs = to_internal(*in_layout)->Get();
        assert(rs);
        pso_desc.pRootSignature = rs;
        d3d12_pipeline->root_signature = rs;
    }

    auto make_d3d12_rasterizer_desc = [](const RasterizerDesc& r)
    {
        return D3D12_RASTERIZER_DESC{
            // Directly compatible
            static_cast<D3D12_FILL_MODE>(r.fill_mode),
            static_cast<D3D12_CULL_MODE>(r.cull_mode),
            r.front_counter_clockwise,
            r.depth_bias,
            r.depth_bias_clamp,
            r.slope_scaled_depth_bias,
            r.depth_clip_enable,
            FALSE, // MultisampleEnable
            FALSE, // AntialiasedLineEnable
            0,     // ForcedSampleCount
            D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
        };
    };

    pso_desc.RasterizerState = make_d3d12_rasterizer_desc(desc.rasterizer_state.value_or(RasterizerDesc{}));

    if (desc.blend_desc.has_value())
    {
        pso_desc.BlendState = {
            .AlphaToCoverageEnable = desc.blend_desc->alpha_to_coverage_enable,
            .IndependentBlendEnable = desc.blend_desc->independent_blend_enable,
        };
        for (unsigned i = 0; i < desc.num_render_targets; i++)
        {
            const auto& rt_blend_desc = desc.blend_desc->render_target[i];
            pso_desc.BlendState.RenderTarget[i] = D3D12_RENDER_TARGET_BLEND_DESC{
                .BlendEnable = rt_blend_desc.blend_enable,
                .LogicOpEnable = rt_blend_desc.logic_op_enable,
                .SrcBlend = blend(rt_blend_desc.src_blend),
                .DestBlend = blend(rt_blend_desc.dst_blend),
                // Directly compatible
                .BlendOp = static_cast<D3D12_BLEND_OP>(rt_blend_desc.blend_op),
                .SrcBlendAlpha = blend(rt_blend_desc.src_blend_alpha),
                .DestBlendAlpha = blend(rt_blend_desc.dst_blend_alpha),
                .BlendOpAlpha = static_cast<D3D12_BLEND_OP>(rt_blend_desc.blend_op_alpha),
                .LogicOp = logic_op(rt_blend_desc.logic_op),
                .RenderTargetWriteMask = rt_blend_desc.render_target_write_mask,
            };
        }
    }
    else
    {
        pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
        pso_desc.BlendState.IndependentBlendEnable = FALSE;
        constexpr D3D12_RENDER_TARGET_BLEND_DESC default_render_target_blend_desc = {
            FALSE,
            FALSE,
            D3D12_BLEND_ONE,
            D3D12_BLEND_ZERO,
            D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE,
            D3D12_BLEND_ZERO,
            D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        };
        for (auto& i : pso_desc.BlendState.RenderTarget)
        {
            i = default_render_target_blend_desc;
        }
    }

    if (desc.depth_stencil_state.has_value())
    {
        auto& depth_stencil_state = desc.depth_stencil_state.value();
        pso_desc.DepthStencilState = {
            .DepthEnable = static_cast<INT>(depth_stencil_state.depth_enable),
            // Directly compatible since mask is binary 0 or 1
            .DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(depth_stencil_state.depth_write_enable),
            .DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(depth_stencil_state.depth_func),
            .StencilEnable = depth_stencil_state.stencil_enable,
            .StencilReadMask = depth_stencil_state.stencil_read_mask,
            .StencilWriteMask = depth_stencil_state.stencil_write_mask,
            // Directly compatible
            .FrontFace =
                {
                    .StencilFailOp = static_cast<D3D12_STENCIL_OP>(depth_stencil_state.front_face.fail_op),
                    .StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(depth_stencil_state.front_face.depth_fail_op),
                    .StencilPassOp = static_cast<D3D12_STENCIL_OP>(depth_stencil_state.front_face.pass_op),
                    .StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(depth_stencil_state.front_face.func),
                },
            .BackFace =
                {
                    .StencilFailOp = static_cast<D3D12_STENCIL_OP>(depth_stencil_state.back_face.fail_op),
                    .StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(depth_stencil_state.back_face.depth_fail_op),
                    .StencilPassOp = static_cast<D3D12_STENCIL_OP>(depth_stencil_state.back_face.pass_op),
                    .StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(depth_stencil_state.back_face.func),
                },
        };
    }
    else
    {
        pso_desc.DepthStencilState.DepthEnable = FALSE;
        pso_desc.DepthStencilState.StencilEnable = FALSE;
    }

    pso_desc.SampleMask = UINT_MAX;

    D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type;
    d3d12_pipeline->primitive_topology = static_cast<D3D12_PRIMITIVE_TOPOLOGY>(desc.topology);

    switch (desc.topology)
    {
    case POINT_LIST:
        topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        break;
    case TRIANGLE_LIST:
    case TRIANGLE_STRIP:
        topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    case LINE_LIST:
    case LINE_STRIP:
        topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    default:
        // This should be exhaustive
        assert(false);
        topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }

    pso_desc.PrimitiveTopologyType = topology_type;

    pso_desc.NumRenderTargets = desc.num_render_targets;

    pso_desc.SampleDesc = DXGI_SAMPLE_DESC{desc.sample_count, 0};

    for (unsigned i = 0; i < desc.num_render_targets; i++)
    {
        pso_desc.RTVFormats[i] = dxgi_format(desc.rtv_formats[i]);
    }
    pso_desc.DSVFormat = dxgi_format(desc.dsv_format);
    if (const auto hr = m_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&d3d12_pipeline->pipeline_state));
        FAILED(hr))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create Graphics Pipeline State\n");
        pipeline->internal_state.reset();
        return false;
    }

    set_debug_name(d3d12_pipeline->pipeline_state.Get(), debug_name);
    return true;
}

bool D3D12Context::bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline)
{
    const auto d3d12_pipeline = to_internal(pipeline);
    const auto cmd_list_d3d12 = to_internal(*cmd_list);

    const auto pipeline_rs = d3d12_pipeline->root_signature.Get();
    assert(pipeline_rs);
    if (cmd_list_d3d12->root_signature != pipeline_rs)
    {
        cmd_list_d3d12->list.Get()->SetGraphicsRootSignature(pipeline_rs);
        cmd_list_d3d12->root_signature = pipeline_rs;
    }

    cmd_list_d3d12->list.Get()->IASetPrimitiveTopology(d3d12_pipeline->primitive_topology);
    cmd_list_d3d12->list.Get()->SetPipelineState(d3d12_pipeline->pipeline_state.Get());

    return true;
}

bool D3D12Context::create_pipeline_layout(PipelineLayoutDesc* const desc, PipelineLayout* const layout)
{
    layout->internal_state = mkS<ComPtr<ID3D12RootSignature>>();
    const auto root_signature = to_internal(*layout);

    auto count_non_empty = [](const std::array<std::vector<LayoutBinding>, MAX_SPACES>& spaces)
    {
        unsigned count = 0;
        for (const auto& space : spaces)
        {
            if (!space.empty())
            {
                ++count;
            }
        }
        return count;
    };
    const auto spaces = count_non_empty(desc->spaces);

    const UINT param_count = desc->push_ranges.size() + spaces;

    assert(param_count <= MAX_SPACES);

    thread_local memory::Arena arena(util::MEGABYTE);
    arena.reset();

    D3D12_ROOT_PARAMETER* params = nullptr;
    if (param_count > 0)
    {
        params = arena.alloc_array<D3D12_ROOT_PARAMETER>(param_count);
    }
    unsigned params_index = 0;

    unsigned size = 0;
    for (unsigned i = 0; i < desc->push_ranges.size(); i++)
    {
        const auto& range = desc->push_ranges[i];
        if (range.size % 4u != 0)
        {
            return false;
        }
        params[params_index++] = {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants =
                {
                    .ShaderRegister = range.binding,
                    .RegisterSpace = range.space,
                    .Num32BitValues = util::ceil_div(range.size, 4u),
                },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
        };
        size += range.size;
        if (size > 128)
        {
            // This breaks Vulkan compability
            OutputDebugStringA(
                "Qhenki D3D12 ERROR: Root constant exceeds 128 bytes which breaks compability with Vulkan\n");
            return false;
        }
    }

    const auto ranges = arena.alloc_array<D3D12_DESCRIPTOR_RANGE*>(desc->spaces.size());

    for (unsigned i = 0; i < desc->spaces.size(); i++)
    {
        auto& space = desc->spaces[i];
        if (space.empty())
        {
            continue;
        }
        // Sort vector of LayoutBindings by binding register
        std::ranges::sort(space,
                          [](const LayoutBinding& a, const LayoutBinding& b)
                          {
                              return a.binding < b.binding;
                          });
        // Assemble ranges dynamically
        const auto l_ranges = arena.alloc_array<D3D12_DESCRIPTOR_RANGE>(space.size());

        unsigned offset = 0;
        for (unsigned j = 0; j < space.size(); j++)
        {
            const auto& binding = space[j];
            // Check that this is not the last binding and not infinite register count
            assert(j == space.size() - 1 || binding.count != INFINITE_DESCRIPTORS);

            // Directly compatible
            const auto type = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(binding.type);

            const D3D12_DESCRIPTOR_RANGE range{
                .RangeType = type,
                .NumDescriptors = binding.count,
                .BaseShaderRegister = binding.binding,
                .RegisterSpace = i,
                .OffsetInDescriptorsFromTableStart = offset,
            };
            offset += binding.count;
            l_ranges[j] = range;
        }
        params[params_index++] = {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable =
                {
                    .NumDescriptorRanges = static_cast<UINT>(space.size()),
                    .pDescriptorRanges = l_ranges,
                },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
        };
        ranges[i] = l_ranges;
    }

    const D3D12_ROOT_SIGNATURE_DESC root_sig_desc{
        // Default range flags
        .NumParameters = param_count,
        .pParameters = params,
        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    };

    ComPtr<ID3DBlob> root_sig_blob, error_blob;
    const auto hr =
        D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_sig_blob, &error_blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to serialize root signature\n");
        // Output error blob
        if (error_blob)
        {
            OutputDebugStringA(static_cast<char*>(error_blob->GetBufferPointer()));
        }
        layout->internal_state.reset();
        return false;
    }
    const void* root_sig_data = root_sig_blob->GetBufferPointer();
    const size_t root_sig_data_size = root_sig_blob->GetBufferSize();

    if (FAILED(m_device->CreateRootSignature(
            0, root_sig_data, root_sig_data_size, IID_PPV_ARGS(root_signature->ReleaseAndGetAddressOf()))))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create root signature\n");
        layout->internal_state.reset();
        return false;
        ;
    }

    return true;
}

bool D3D12Context::set_pipeline_constant(CommandList* cmd_list,
                                         const PipelineLayout& expected_layout,
                                         const unsigned param,
                                         const uint32_t offset,
                                         const unsigned size,
                                         void* data)
{
    if (size % 4u != 0)
    {
        return false;
    }
    if (offset % 4u != 0)
    {
        return false;
    }

    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto layout_d3d12 = to_internal(expected_layout);

    if (cmd_list_d3d12->root_signature != layout_d3d12->Get())
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Root signature mismatch when setting pipeline constant\n");
        return false;
    }

    cmd_list_d3d12->list.Get()->SetGraphicsRoot32BitConstants(param, size / 4u, data, offset / 4u);
    return true;
}

bool D3D12Context::create_descriptor_heap(const DescriptorHeapDesc& desc,
                                          DescriptorHeap* const heap,
                                          const char* debug_name)
{
    if (desc.visibility == DescriptorHeapDesc::Visibility::GPU)
    {
        switch (m_capabilities.options.ResourceBindingTier)
        {
        case D3D12_RESOURCE_BINDING_TIER_1:
            assert(false); // Should never happen here because we require Tier 2, but limit is same as Tier 2
        case D3D12_RESOURCE_BINDING_TIER_2:
            if (desc.num_descriptors > D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2)
            {
                OutputDebugStringA("Qhenki D3D12: Descriptor count exceeds maximum for Resource Binding Tier\n");
                return false;
            }
            // No break since sampler check is the same for both Tier 2 and 3
        case D3D12_RESOURCE_BINDING_TIER_3:
            // There is no bound here for CBV/SRV/UAV
            if (desc.type == DescriptorHeapDesc::Type::SAMPLER &&
                desc.num_descriptors > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE)
            {
                OutputDebugStringA("Qhenki D3D12: Descriptor count exceeds maximum for Resource Binding Tier\n");
                return false;
            }
            break;
        }
    }

    heap->internal_state = mkS<D3D12DescriptorHeap>();
    auto d3d12_heap = to_internal(*heap);
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{
        .NumDescriptors = desc.num_descriptors,
    };

    switch (desc.type)
    {
    case DescriptorHeapDesc::Type::CBV_SRV_UAV:
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        break;
    case DescriptorHeapDesc::Type::SAMPLER:
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        break;
    default:
        OutputDebugStringA("Qhenki D3D12: Invalid descriptor heap type\n");
        heap->internal_state.reset();
        return false;
    }

    if (desc.visibility == DescriptorHeapDesc::Visibility::CPU)
    {
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    }
    else if (desc.visibility == DescriptorHeapDesc::Visibility::GPU)
    {
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    if (!d3d12_heap->create(m_device.Get(), heap_desc))
    {
        heap->internal_state.reset();
        return false;
    }

    set_debug_name(d3d12_heap->get().Get(), debug_name);
    heap->desc = desc;
    return true;
}

void D3D12Context::set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap)
{
    assert(cmd_list);
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto heap_d3d12 = to_internal(heap);
    if (heap.desc.type == DescriptorHeapDesc::Type::CBV_SRV_UAV || heap.desc.type == DescriptorHeapDesc::Type::SAMPLER)
    {
        cmd_list_d3d12->list.Get()->SetDescriptorHeaps(1, heap_d3d12->get().GetAddressOf());
    }
    else
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Invalid descriptor heap type\n");
    }
}

void D3D12Context::set_descriptor_heap(CommandList* cmd_list,
                                       const DescriptorHeap& heap,
                                       const DescriptorHeap& sampler_heap)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto heap_d3d12 = to_internal(heap);
    const auto sampler_heap_d3d12 = to_internal(sampler_heap);
    if (heap.desc.type == DescriptorHeapDesc::Type::CBV_SRV_UAV)
    {
        const std::array heaps = {heap_d3d12->get().Get(), sampler_heap_d3d12->get().Get()};
        cmd_list_d3d12->list.Get()->SetDescriptorHeaps(heaps.size(), heaps.data());
    }
    else
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Invalid descriptor heap type\n");
    }
}

bool D3D12Context::set_descriptor_table(CommandList* cmd_list,
                                        const PipelineLayout& expected_layout,
                                        const unsigned index,
                                        const Descriptor& gpu_descriptor)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto layout_d3d12 = to_internal(expected_layout);

    if (cmd_list_d3d12->root_signature != layout_d3d12->Get())
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Root signature mismatch when setting descriptor table\n");
        return false;
    }

    const auto heap_d3d12 = to_internal(*gpu_descriptor.heap);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
    if (!heap_d3d12->get_GPU_descriptor(&gpu_handle, gpu_descriptor.offset))
    {
        return false;
    }

    cmd_list_d3d12->list.Get()->SetGraphicsRootDescriptorTable(index, gpu_handle);
    return true;
}

bool D3D12Context::copy_descriptors(const size_t num_descriptors, const Descriptor& src, const Descriptor& dst)
{
    const auto src_heap_d3d12 = to_internal(*src.heap);
    const auto dst_heap_d3d12 = to_internal(*dst.heap);
    const auto& src_desc = src_heap_d3d12->get_desc();
    const auto& dst_desc = dst_heap_d3d12->get_desc();
    if (src_desc.Type != dst_desc.Type)
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Source and destination descriptor heaps must be of the same type\n");
        return false;
    }

    if (src_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Source heap cannot be shader visible\n");
        return false;
    }

    m_descriptor_copier.add_pending_descriptor_copy(num_descriptors, src, dst);

    return true;
}

bool D3D12Context::create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, const char* debug_name)
{
    assert(buffer);
    buffer->internal_state = mkS<ComPtr<D3D12MA::Allocation>>();
    const auto allocation = to_internal(*buffer);

    const auto size = desc.usage & BufferUsage::CONSTANT
                        ? util::align_u(desc.size, static_cast<size_t>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))
                        : desc.size;

    D3D12_RESOURCE_DESC1 resource_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc =
            {
                .Count = 1,
                .Quality = 0,
            },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
        // Don't care about SamplerFeedbackMipRegion
    };
    if (desc.usage & BufferUsage::UAV)
    {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    const auto is_cpu_visible = desc.visibility & CPU_SEQUENTIAL;

    D3D12MA::ALLOCATION_DESC allocation_desc{};
    if (is_cpu_visible)
    {
        allocation_desc.HeapType = desc.visibility & GPU ? D3D12_HEAP_TYPE_GPU_UPLOAD : D3D12_HEAP_TYPE_UPLOAD;
    }
    else
    {
        allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    }

    // Barrier layout is undefined for CreateResource3
    if (FAILED(m_allocator->CreateResource3(&allocation_desc,
                                            &resource_desc,
                                            D3D12_BARRIER_LAYOUT_UNDEFINED,
                                            nullptr,
                                            0,
                                            nullptr,
                                            allocation->ReleaseAndGetAddressOf(),
                                            IID_NULL,
                                            NULL)))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create buffer\n");
        buffer->internal_state.reset();
        return false;
    }
    const auto resource = allocation->Get()->GetResource();

    if (data)
    {
        if (is_cpu_visible)
        {
            constexpr D3D12_RANGE range(0, 0);
            void* mapped_ptr;
            if (FAILED(resource->Map(0, &range, &mapped_ptr)))
            {
                OutputDebugStringA("Qhenki D3D12 ERROR: Failed to map buffer\n");
                buffer->internal_state.reset();
                return false;
            }
            memcpy(mapped_ptr, data, desc.size);
            resource->Unmap(0, nullptr);
        }
        else
        {
            OutputDebugStringA("Qhenki D3D12 WARNING: Tried to initialize non CPU visible buffer with data\n");
        }
    }

    set_debug_name(allocation->Get()->GetResource(), debug_name);

    buffer->desc = desc;
    buffer->desc.size = size;
    return true;
}

bool D3D12Context::create_descriptor_constant_view(const Buffer& buffer,
                                                   DescriptorHeap* const heap,
                                                   Descriptor* descriptor)
{
    if (heap->desc.type != DescriptorHeapDesc::Type::CBV_SRV_UAV)
    {
        return false;
    }

    if (buffer.desc.size % 16 != 0)
    {
        OutputDebugStringA(
            "Qhenki D3D12 ERROR: Buffer size is not a multiple of 16 bytes, cannot create constant buffer view\n");
        return false;
    }

    const auto buffer_d3d12 = to_internal(buffer);
    const auto heap_d3d12 = to_internal(*heap);

    const D3D12_CONSTANT_BUFFER_VIEW_DESC desc{
        .BufferLocation = buffer_d3d12->Get()->GetResource()->GetGPUVirtualAddress(),
        .SizeInBytes = static_cast<UINT>(buffer.desc.size),
    };

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    heap_d3d12->get_CPU_descriptor(&cpu_handle, descriptor->offset);
    m_device->CreateConstantBufferView(&desc, cpu_handle);

    descriptor->heap = heap;

    return true;
}

bool D3D12Context::create_descriptor_shader_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor)
{
    if (heap->desc.type != DescriptorHeapDesc::Type::CBV_SRV_UAV)
    {
        return false;
    }

    const auto is_raw = buffer.desc.stride == 0;
    const auto num_elements = is_raw ? buffer.desc.size / 4u : buffer.desc.size / buffer.desc.stride;

    assert(num_elements < std::numeric_limits<UINT>::max());

    const D3D12_SHADER_RESOURCE_VIEW_DESC desc{
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer =
            {
                .FirstElement = 0,
                .NumElements = static_cast<UINT>(num_elements),
                .StructureByteStride = static_cast<UINT>(buffer.desc.stride),
                .Flags = is_raw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE,
            },
    };

    const auto buffer_d3d12 = to_internal(buffer);
    const auto heap_d3d12 = to_internal(*heap);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    heap_d3d12->get_CPU_descriptor(&cpu_handle, descriptor->offset);
    m_device->CreateShaderResourceView(buffer_d3d12->Get()->GetResource(), &desc, cpu_handle);

    descriptor->heap = heap;

    return true;
}

void D3D12Context::copy_buffer(CommandList* cmd_list,
                               const Buffer& src,
                               const uint64_t src_offset,
                               Buffer* dst,
                               const uint64_t dst_offset,
                               const uint64_t bytes)
{
    assert(src_offset + bytes <= src.desc.size);
    assert(dst_offset + bytes <= dst->desc.size);
    const auto src_allocation = to_internal(src);
    const auto dst_allocation = to_internal(*dst);
    const auto src_resource = src_allocation->Get()->GetResource();
    const auto dst_resource = dst_allocation->Get()->GetResource();

    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    cmd_list_d3d12->list.Get()->CopyBufferRegion(dst_resource, dst_offset, src_resource, src_offset, bytes);
}

bool D3D12Context::create_texture(const TextureDesc& desc, Texture* texture, const char* debug_name)
{
    assert(texture);
    if (desc.height > 1 && desc.dimension == TextureDimension::TEXTURE_1D)
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Tried to initialize 1D texture with height > 1\n");
        return false;
    }
    if (desc.usage == TextureDesc::NONE)
    {
        return false;
    }
    if (desc.is_cube)
    {
        // Cubemaps must be 2D textures with square faces and array size divisible by 6
        if (desc.dimension != TextureDimension::TEXTURE_2D || desc.width != desc.height ||
            (desc.depth_or_array_size % 6) != 0)
        {
            return false;
        }
    }

    const auto format = dxgi_format(desc.format);

    D3D12_RESOURCE_DESC1 resource_desc = {
        .Alignment = 0,
        .Width = desc.width,
        .Height = desc.height,
        .DepthOrArraySize = desc.depth_or_array_size,
        .MipLevels = desc.mip_levels,
        .Format = format,
        .SampleDesc =
            {
                .Count = desc.sample_count,
                .Quality = 0,
            },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
        //.SamplerFeedbackMipRegion // TODO: sampler feedback mip region?
    };
    D3D12_CLEAR_VALUE clear{
        .Format = format,
    };
    const D3D12_CLEAR_VALUE* clear_ptr = nullptr;

    if (desc.usage & TextureDesc::DEPTH_STENCIL)
    {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if (desc.initial_layout == Layout::DEPTH_STENCIL_WRITE)
        {
            clear_ptr = &clear;
            clear.DepthStencil = {.Depth = desc.clear_depth_value.depth, .Stencil = desc.clear_depth_value.stencil};
        }
    }
    if (desc.usage & TextureDesc::RENDER_TARGET)
    {
        if (clear_ptr)
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Texture cannot have both depth stencil and render target usage\n");
            return false;
        }
        clear_ptr = &clear;
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        static_assert(std::tuple_size_v<decltype(std::declval<TextureDesc>().clear_color_value)> == 4);
        for (unsigned i = 0; i < 4; i++)
        {
            clear.Color[i] = desc.clear_color_value[i];
        }
    }
    if (desc.usage & TextureDesc::UNORDERED_ACCESS)
    {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    switch (desc.dimension)
    {
    case TextureDimension::TEXTURE_1D:
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        break;
    case TextureDimension::TEXTURE_2D:
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        break;
    case TextureDimension::TEXTURE_3D:
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        break;
    }

    constexpr D3D12MA::ALLOCATION_DESC allocation_desc{
        .HeapType = D3D12_HEAP_TYPE_DEFAULT,
    };

    texture->internal_state = mkS<D3D12Texture>();
    const auto texture_d3d12 = to_internal(*texture);

    if (FAILED(m_allocator->CreateResource3(&allocation_desc,
                                            &resource_desc,
                                            layout(desc.initial_layout),
                                            clear_ptr,
                                            0,
                                            nullptr, // Probably not going to cast
                                            texture_d3d12->allocation.ReleaseAndGetAddressOf(),
                                            IID_NULL,
                                            NULL)))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create texture\n");
        texture->internal_state.reset();
        return false;
    }

    set_debug_name(texture_d3d12->allocation.Get()->GetResource(), debug_name);

    texture->desc = desc;
    return true;
}

bool D3D12Context::create_descriptor_shader_view(const Texture& texture,
                                                 DescriptorHeap* const heap,
                                                 Descriptor* descriptor)
{
    const auto texture_d3d12 = to_internal(texture);
    const auto heap_d3d12 = to_internal(*heap);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    heap_d3d12->get_CPU_descriptor(&cpu_handle, descriptor->offset);

    m_device->CreateShaderResourceView(texture_d3d12->allocation.Get()->GetResource(), nullptr, cpu_handle);

    descriptor->heap = heap;

    return true;
}

bool D3D12Context::copy_to_texture(CommandList* cmd_list,
                                   const void* data,
                                   Buffer* const staging,
                                   Texture* const texture)
{
    const UINT num_subresources = texture->desc.mip_levels * texture->desc.depth_or_array_size;
    const auto texture_allocation = to_internal(*texture);
    const auto desc = texture_allocation->allocation.Get()->GetResource()->GetDesc();

    auto& arena = acquire_arena(m_frame_count);
    const auto layouts = arena.alloc_array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(num_subresources);
    const auto row_counts = arena.alloc_array<UINT>(num_subresources);
    const auto row_sizes = arena.alloc_array<UINT64>(num_subresources);

    UINT64 size;
    m_device->GetCopyableFootprints(&desc, 0, num_subresources, 0, layouts, row_counts, row_sizes, &size);

    const BufferDesc staging_desc{
        .size = size,
        .usage = BufferUsage::COPY_SRC,
        .visibility = CPU_SEQUENTIAL,
    };

    // TODO: Transient staging buffer
    Buffer local_staging;
    if (!create_buffer(staging_desc, nullptr, &local_staging, nullptr))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create staging buffer for texture copy\n");
        return false;
    }

    const auto upload_memory = static_cast<uint8_t*>(map_buffer(local_staging));

    size_t data_offset = 0;

    for (UINT subresource = 0; subresource < num_subresources; subresource++)
    {
        const UINT32 mip = subresource % texture->desc.mip_levels;

        const UINT32 mip_width = std::max(1u, texture->desc.width >> mip);
        const UINT32 mip_height = std::max(1u, texture->desc.height >> mip);
        // For 3D textures, depth varies per mip. For arrays/cubemaps, always 1 (one 2D slice per subresource)
        const UINT32 mip_depth = desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D
                                   ? std::max<UINT32>(1u, texture->desc.depth_or_array_size >> mip)
                                   : 1u;

        size_t src_row_pitch = 0;
        size_t src_slice_pitch = 0;
        if (FAILED(
                ComputePitch(dxgi_format(texture->desc.format), mip_width, mip_height, src_row_pitch, src_slice_pitch)))
        {
            return false;
        }

        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = layouts[subresource];
        const UINT num_rows = row_counts[subresource];
        const UINT64 row_size_bytes = row_sizes[subresource];
        const UINT32 dst_row_pitch = footprint.Footprint.RowPitch;
        const UINT32 dst_slice_bytes = dst_row_pitch * num_rows;

        UINT8* dst = &upload_memory[footprint.Offset];
        const UINT8* src = static_cast<const UINT8*>(data) + data_offset;

        for (UINT32 z = 0; z < mip_depth; z++) // 1 for 2D or array textures
        {
            UINT8* dst_slice = &dst[z * dst_slice_bytes];
            const UINT8* src_slice = &src[z * src_slice_pitch];

            for (UINT y = 0; y < num_rows; y++)
            {
                memcpy(&dst_slice[y * dst_row_pitch], &src_slice[y * src_row_pitch], row_size_bytes);
            }
        }

        data_offset += src_slice_pitch * mip_depth;
    }

    unmap_buffer(local_staging);

    const auto staging_internal = to_internal(local_staging);
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    for (UINT subresource_index = 0; subresource_index < num_subresources; subresource_index++)
    {
        D3D12_TEXTURE_COPY_LOCATION destination{
            .pResource = texture_allocation->allocation.Get()->GetResource(),
            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            .SubresourceIndex = subresource_index,
        };
        D3D12_TEXTURE_COPY_LOCATION source{
            .pResource = staging_internal->Get()->GetResource(),
            .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
            .PlacedFootprint = layouts[subresource_index],
            // PlacedFootprint offset is 0 since handled by allocator
        };
        cmd_list_d3d12->list.Get()->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    }

    *staging = std::move(local_staging);

    return true;
}

bool D3D12Context::create_descriptor(const SamplerDesc& desc, DescriptorHeap* const heap, Descriptor* descriptor)
{
    if (heap->desc.type != DescriptorHeapDesc::Type::SAMPLER)
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Invalid descriptor heap type\n");
        return false;
    }

    const D3D12_SAMPLER_DESC sampler_desc{
        .Filter =
            filter(desc.min_filter, desc.mag_filter, desc.mip_filter, desc.comparison_enable, desc.max_anisotropy),
        // Directly compatible
        .AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(desc.address_mode_u),
        .AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(desc.address_mode_v),
        .AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(desc.address_mode_w),
        .MipLODBias = desc.mip_lod_bias,
        .MaxAnisotropy = desc.max_anisotropy,
        // Directly compatible
        .ComparisonFunc = static_cast<D3D12_COMPARISON_FUNC>(desc.comparison_func),
        .BorderColor = {desc.border_color[0], desc.border_color[1], desc.border_color[2], desc.border_color[3]},
        .MinLOD = desc.min_lod,
        .MaxLOD = desc.max_lod,
    };

    const auto heap_d3d12 = to_internal(*heap);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    heap_d3d12->get_CPU_descriptor(&cpu_handle, descriptor->offset);
    m_device->CreateSampler(&sampler_desc, cpu_handle);

    descriptor->heap = heap;

    return true;
}

void* D3D12Context::map_buffer(const Buffer& buffer)
{
    // Check if buffer is CPU visible
    if (buffer.desc.visibility & CPU_SEQUENTIAL)
    {
        const auto allocation = to_internal(buffer);
        const auto resource = allocation->Get()->GetResource();
        constexpr D3D12_RANGE range(0, 0);
        void* mapped_ptr;
        const auto result = resource->Map(0, &range, &mapped_ptr);
        if (FAILED(result))
        {
            __debugbreak();
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to map buffer\n");
            return nullptr;
        }
        return mapped_ptr;
    }
    OutputDebugStringA("Qhenki D3D12 ERROR: Buffer is not CPU visible\n");

    return nullptr;
}

void D3D12Context::unmap_buffer(const Buffer& buffer)
{
    if (buffer.desc.visibility & CPU_SEQUENTIAL)
    {
        const auto allocation = to_internal(buffer);
        const auto resource = allocation->Get()->GetResource();
        resource->Unmap(0, nullptr);
    }
    else
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Buffer is not CPU visible\n");
    }
}

bool D3D12Context::bind_vertex_buffers(CommandList* cmd_list,
                                       const unsigned start_slot,
                                       const unsigned buffer_count,
                                       const Buffer* const* buffers,
                                       const uint64_t* sizes,
                                       const uint64_t* strides,
                                       const uint64_t* offsets)
{
    if (buffer_count > D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)
    {
        return false;
    }

    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto command_list = cmd_list_d3d12->list.Get();

    std::array<D3D12_VERTEX_BUFFER_VIEW, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> vertex_buffer_views;
    for (unsigned i = 0; i < buffer_count; i++)
    {
        const auto allocation = to_internal(*buffers[i]);
        const auto resource = allocation->Get()->GetResource();
        assert(sizes[i] <= std::numeric_limits<UINT>::max());
        assert(strides[i] <= std::numeric_limits<UINT>::max());
        vertex_buffer_views[i] = {
            .BufferLocation = resource->GetGPUVirtualAddress() + offsets[i],
            .SizeInBytes = static_cast<UINT>(sizes[i]),
            .StrideInBytes = static_cast<UINT>(strides[i]),
        };
    }

    command_list->IASetVertexBuffers(start_slot, buffer_count, vertex_buffer_views.data());
    return true;
}

void D3D12Context::bind_index_buffer(CommandList* cmd_list,
                                     const Buffer& buffer,
                                     const IndexType format,
                                     uint64_t offset)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto command_list = cmd_list_d3d12->list.Get();

    const auto allocation = to_internal(buffer);
    const auto resource = allocation->Get()->GetResource();
    const D3D12_INDEX_BUFFER_VIEW view = {
        .BufferLocation = resource->GetGPUVirtualAddress() + offset,
        .SizeInBytes = static_cast<UINT>(buffer.desc.size - offset),
        .Format = dxgi_format(format),
    };

    command_list->IASetIndexBuffer(&view);
}

bool D3D12Context::create_command_pool(CommandPool* command_pool, const QueueType queue, const char* debug_name)
{
    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    switch (queue)
    {
    case GRAPHICS:
        type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        break;
    case COMPUTE:
        type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        break;
    case COPY:
        type = D3D12_COMMAND_LIST_TYPE_COPY;
        break;
    }

    command_pool->internal_state = mkS<ComPtr<ID3D12CommandAllocator>>();
    const auto command_allocator = to_internal(*command_pool);

    if (FAILED(m_device->CreateCommandAllocator(type, IID_PPV_ARGS(command_allocator->ReleaseAndGetAddressOf()))))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create command allocator\n");
        command_pool->internal_state.reset();
        return false;
    }
    command_pool->queue_type = queue;

    set_debug_name(command_allocator->Get(), debug_name);

    return true;
}

bool D3D12Context::reset_command_list(CommandList* cmd_list, const CommandPool& command_pool)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    if (FAILED(cmd_list_d3d12->list.Get()->Reset(to_internal(command_pool)->Get(), nullptr)))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to reset command list");
        return false;
    }
    cmd_list_d3d12->root_signature = nullptr;
    return true;
}

bool D3D12Context::create_command_list(CommandList* cmd_list, const CommandPool& command_pool, const char* debug_name)
{
    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    switch (command_pool.queue_type)
    {
    case GRAPHICS:
        type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        break;
    case COMPUTE:
        type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        break;
    case COPY:
        type = D3D12_COMMAND_LIST_TYPE_COPY;
        break;
    }

    cmd_list->internal_state = mkS<D3D12CommandList>();
    if (FAILED(m_device->CreateCommandList1(0,
                                            type,
                                            D3D12_COMMAND_LIST_FLAG_NONE,
                                            IID_PPV_ARGS(to_internal(*cmd_list)->list.ReleaseAndGetAddressOf()))))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create command list\n");
        cmd_list->internal_state.reset();
        return false;
    }

    return true;
}

bool D3D12Context::close_command_list(CommandList* cmd_list)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    if (FAILED(cmd_list_d3d12->list.Get()->Close()))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to close command list\n");
        return false;
    }
    return true;
}

bool D3D12Context::reset_command_pool(CommandPool* command_pool)
{
    const auto command_allocator = to_internal(*command_pool);
    if (FAILED(command_allocator->Get()->Reset()))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to reset command allocator\n");
        return false;
    }
    return true;
}

ID3D12CommandQueue* D3D12Context::get_command_queue(const QueueType queue) const
{
    switch (queue)
    {
    case GRAPHICS:
        return m_graphics_queue.Get();
    case COMPUTE:
        return m_compute_queue.Get();
    case COPY:
        return m_copy_queue.Get();
    default:
        assert(false);
        return nullptr;
    }
}

namespace
{
RenderTargetHelper& get_render_target_helper(ID3D12Device* device, HRESULT* success)
{
    thread_local RenderTargetHelper helper;
    *success = helper.Init(device);
    return helper;
}

void clear_depth(ID3D12GraphicsCommandList7* command_list,
                 const RenderTarget* const depth_stencil,
                 RenderTargetHelper* const render_target_helper)
{
    if (depth_stencil)
    {
        assert(depth_stencil->texture->desc.usage & TextureDesc::DEPTH_STENCIL);
        if (depth_stencil->clear_type != RenderTarget::NONE)
        {
            auto clear_flags = static_cast<D3D12_CLEAR_FLAGS>(0);
            if (depth_stencil->clear_type & RenderTarget::DEPTH)
            {
                clear_flags |= D3D12_CLEAR_FLAG_DEPTH;
            }
            if (depth_stencil->clear_type & RenderTarget::STENCIL)
            {
                clear_flags |= D3D12_CLEAR_FLAG_STENCIL;
            }
            assert(clear_flags);
            auto [clear_depth_value, clear_stencil_value] = depth_stencil->clear_params.dsv_clear_params;

            const auto dsv_resource = to_internal(*depth_stencil->texture)->allocation.Get()->GetResource();

            render_target_helper->ClearDepthStencilView(
                command_list, dsv_resource, nullptr, clear_flags, clear_depth_value, clear_stencil_value, 0, nullptr);
        }
    }
}
} // namespace

bool D3D12Context::start_render_pass(CommandList* cmd_list,
                                     const float* clear_color_values,
                                     const RenderTarget* const depth_stencil)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto command_list = cmd_list_d3d12->list.Get();

    HRESULT success;
    auto& render_target_helper = get_render_target_helper(m_device.Get(), &success);
    if (FAILED(success))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to initialize render target helper\n");
        return false;
    }

    ID3D12Resource* dsv = nullptr;
    if (depth_stencil)
    {
        dsv = to_internal(*depth_stencil->texture)->allocation.Get()->GetResource();
    }
    clear_depth(command_list, depth_stencil, &render_target_helper);

    render_target_helper.OMSetRenderTargets(
        command_list, 1, m_swapchain_buffers[m_swapchain_index].GetAddressOf(), nullptr, dsv, nullptr);

    if (clear_color_values)
    {
        render_target_helper.ClearRenderTargetView(
            command_list, m_swapchain_buffers[m_swapchain_index].Get(), nullptr, clear_color_values, 0, nullptr);
    }

    return true;
}

bool D3D12Context::start_render_pass(CommandList* cmd_list,
                                     const unsigned rt_count,
                                     const RenderTarget* rts,
                                     const RenderTarget* const depth_stencil)
{
    if (rt_count > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        return false;
    }

    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto command_list = cmd_list_d3d12->list.Get();

    std::array<ID3D12Resource*, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT> d3d12_textures;
    for (unsigned i = 0; i < rt_count; i++)
    {
        assert(rts[i].texture->desc.usage & TextureDesc::RENDER_TARGET);
        d3d12_textures[i] = to_internal(*rts[i].texture)->allocation.Get()->GetResource();
    }

    ID3D12Resource* dsv = nullptr;
    if (depth_stencil)
    {
        dsv = to_internal(*depth_stencil->texture)->allocation.Get()->GetResource();
    }

    HRESULT success;
    auto& render_target_helper = get_render_target_helper(m_device.Get(), &success);
    if (FAILED(success))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to initialize render target helper\n");
        return false;
    }

    render_target_helper.OMSetRenderTargets(command_list, rt_count, d3d12_textures.data(), nullptr, dsv, nullptr);

    for (unsigned i = 0; i < rt_count; i++)
    {
        if (rts[i].clear_type == RenderTarget::COLOR)
        {
            const auto d3d12_tex = to_internal(*rts[i].texture);
            render_target_helper.ClearRenderTargetView(command_list,
                                                       d3d12_tex->allocation.Get()->GetResource(),
                                                       nullptr,
                                                       rts[i].clear_params.clear_color_value.data(),
                                                       0,
                                                       nullptr);
        }
    }
    clear_depth(command_list, depth_stencil, &render_target_helper);

    return true;
}

void D3D12Context::set_viewports(CommandList* list, const unsigned count, const Viewport* viewport)
{
    std::array<D3D12_VIEWPORT, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> d3d12_viewports;
    for (unsigned i = 0; i < count; i++)
    {
        d3d12_viewports[i] = {
            .TopLeftX = viewport[i].top_left_x,
            .TopLeftY = viewport[i].top_left_y,
            .Width = viewport[i].width,
            .Height = viewport[i].height,
            .MinDepth = viewport[i].min_depth,
            .MaxDepth = viewport[i].max_depth,
        };
    }

    const auto cmd_list_d3d12 = to_internal(*list);
    const auto command_list = cmd_list_d3d12->list.Get();
    command_list->RSSetViewports(count, d3d12_viewports.data());
}

void D3D12Context::set_scissor_rects(CommandList* list, const unsigned count, const Rect* scissor_rect)
{
    std::array<D3D12_RECT, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> d3d12_rects;
    for (unsigned i = 0; i < count; i++)
    {
        d3d12_rects[i] = {
            .left = scissor_rect[i].left,
            .top = scissor_rect[i].top,
            .right = scissor_rect[i].left + static_cast<int32_t>(scissor_rect[i].width),
            .bottom = scissor_rect[i].top + static_cast<int32_t>(scissor_rect[i].height),
        };
    }

    const auto cmd_list_d3d12 = to_internal(*list);
    const auto command_list = cmd_list_d3d12->list.Get();
    command_list->RSSetScissorRects(count, d3d12_rects.data());
}

void D3D12Context::end_render_pass(CommandList* cmd_list)
{
}

void D3D12Context::draw(CommandList* cmd_list, const uint32_t vertex_count, const uint32_t start_vertex_offset)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto command_list = cmd_list_d3d12->list.Get();
    command_list->DrawInstanced(vertex_count, 1, start_vertex_offset, 0);
}

void D3D12Context::draw_indexed(CommandList* cmd_list,
                                const uint32_t index_count,
                                const uint32_t instance_count,
                                const uint32_t start_index_offset,
                                const int32_t base_vertex_offset,
                                const uint32_t instance_offset)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto command_list = cmd_list_d3d12->list.Get();
    command_list->DrawIndexedInstanced(
        index_count, instance_count, start_index_offset, base_vertex_offset, instance_offset);
}

bool D3D12Context::submit_command_lists(const SubmitInfo& submit_info, const QueueType queue)
{
    std::scoped_lock lock(m_submit_mutex);
    // Internally ordered within the same queue so treat this as an error
    for (unsigned i = 0; i < submit_info.wait_fence_count; i++)
    {
        if (submit_info.wait_queues[i] == queue)
        {
            return false;
        }
    }
    for (unsigned i = 0; i < submit_info.wait_fence_count; i++)
    {
        const auto fence = to_internal(submit_info.wait_fences[i]);
        const auto result =
            get_command_queue(submit_info.wait_queues[i])->Wait(fence->fence.Get(), submit_info.wait_values[i]);
        if (FAILED(result))
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to wait for fence\n");
            return false;
        }
    }

    auto merge_count = m_descriptor_copier.merge_regions();
    const auto descriptor_regions = m_descriptor_copier.get_merged_regions();
    for (size_t i = 0; i < descriptor_regions.size(); i++)
    {
        const PendingDescriptorCopy& pending = descriptor_regions[i];
        const auto src_heap = to_internal(*pending.src.heap);
        const auto dst_heap = to_internal(*pending.dst.heap);

        D3D12_CPU_DESCRIPTOR_HANDLE src_cpu_handle;
        src_heap->get_CPU_descriptor(&src_cpu_handle, pending.src.offset);

        D3D12_CPU_DESCRIPTOR_HANDLE dst_cpu_handle;
        dst_heap->get_CPU_descriptor(&dst_cpu_handle, pending.dst.offset);

        assert(src_heap->get_desc().Type == dst_heap->get_desc().Type);
        const auto size = pending.src.heap->desc.type == DescriptorHeapDesc::Type::CBV_SRV_UAV
                            ? m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
                            : m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        m_device->CopyDescriptorsSimple(pending.descriptors, dst_cpu_handle, src_cpu_handle, src_heap->get_desc().Type);
    }
    m_descriptor_copier.reset();

    auto& arena = acquire_arena(m_frame_count);
    const auto cmd_list_ptrs = arena.alloc_array<ID3D12CommandList*>(submit_info.command_list_count);

    for (unsigned i = 0; i < submit_info.command_list_count; i++)
    {
        const auto cmd_list_d3d12 = to_internal(submit_info.command_lists[i]);
        cmd_list_ptrs[i] = cmd_list_d3d12->list.Get();
    }

    const auto q = get_command_queue(queue);

    q->ExecuteCommandLists(submit_info.command_list_count, cmd_list_ptrs);

    for (unsigned i = 0; i < submit_info.signal_fence_count; i++)
    {
        const auto fence = to_internal(submit_info.signal_fences[i]);
        const auto result = q->Signal(fence->fence.Get(), submit_info.signal_values[i]);
        if (FAILED(result))
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to signal fence\n");
            return false;
        }
    }

    return true;
}

bool D3D12Context::create_fence(Fence* fence, const uint64_t initial_value, const char* debug_name)
{
    fence->internal_state = mkS<D3D12Fence>();
    const auto fence_d3d12 = to_internal(*fence);

    if (FAILED(m_device->CreateFence(initial_value,
                                     D3D12_FENCE_FLAG_NONE,
                                     IID_PPV_ARGS(fence_d3d12->fence.ReleaseAndGetAddressOf()))))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create fence\n");
        fence->internal_state.reset();
        return false;
    }

    fence_d3d12->event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!fence_d3d12->event)
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create fence event\n");
        fence->internal_state.reset();
        return false;
    }

    set_debug_name(fence_d3d12->fence.Get(), debug_name);

    return true;
}

uint64_t D3D12Context::get_fence_value(const Fence& fence)
{
    const auto fence_d3d12 = to_internal(fence);
    return fence_d3d12->fence->GetCompletedValue();
}

bool D3D12Context::wait_fences(const WaitInfo& info)
{
    auto& arena = acquire_arena(m_frame_count);
    auto wait_handles = arena.alloc_array<HANDLE>(info.count);

    for (unsigned i = 0; i < info.count; i++)
    {
        const auto d3d12_fence = to_internal(info.fences[i]);
        if (FAILED(d3d12_fence->fence->SetEventOnCompletion(info.values[i], d3d12_fence->event)))
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Failed to set event on fence\n");
            return false;
        }
        wait_handles[i] = d3d12_fence->event;
    }
    WaitForMultipleObjectsEx(info.count, wait_handles, info.wait_all, INFINITE, FALSE);
    return true;
}

void D3D12Context::set_barrier_resource(const unsigned count, ImageBarrier* barriers, const Swapchain& swapchain)
{
    for (unsigned i = 0; i < count; i++)
    {
        barriers[i].resource = static_cast<void*>(m_swapchain_buffers[m_swapchain_index].Get());
    }
}

void D3D12Context::set_barrier_resource(const unsigned count, ImageBarrier* barriers, const Texture& render_target)
{
    assert(count == 0 || barriers);
    for (unsigned i = 0; i < count; i++)
    {
        barriers[i].resource = static_cast<void*>(to_internal(render_target)->allocation.Get()->GetResource());
    }
}

bool D3D12Context::issue_barrier(CommandList* cmd_list, const unsigned count, const ImageBarrier* barriers)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    const auto command_list = cmd_list_d3d12->list.Get();

    auto& arena = acquire_arena(m_frame_count);
    auto d3d12_barriers = arena.alloc_array<D3D12_TEXTURE_BARRIER>(count);

    for (unsigned i = 0; i < count; i++)
    {
        const auto& barrier = barriers[i];

        if (!barrier.resource)
        {
            OutputDebugStringA("Qhenki D3D12 ERROR: Barrier resource is null. Barrier was not issued\n");
            return false;
        }

        auto& d3d12_barrier = d3d12_barriers[i];
        d3d12_barrier = {
            .SyncBefore = sync_stage(barrier.src_stage),
            .SyncAfter = sync_stage(barrier.dst_stage),
            .AccessBefore = access_flags(barrier.src_access),
            .AccessAfter = access_flags(barrier.dst_access),
            .LayoutBefore = layout(barrier.src_layout),
            .LayoutAfter = layout(barrier.dst_layout),
            .pResource = static_cast<ID3D12Resource*>(barrier.resource),
            .Subresources =
                {
                    .IndexOrFirstMipLevel = barrier.subresource_range.base_mip_level,
                    .NumMipLevels = barrier.subresource_range.mip_level_count,
                    .FirstArraySlice = barrier.subresource_range.base_array_layer,
                    .NumArraySlices = barrier.subresource_range.array_layer_count,
                    .FirstPlane = 0,
                    .NumPlanes = 1,
                },
            .Flags = barrier.discard ? D3D12_TEXTURE_BARRIER_FLAG_DISCARD : D3D12_TEXTURE_BARRIER_FLAG_NONE,
        };
    }

    const D3D12_BARRIER_GROUP barrier_group = {
        .Type = D3D12_BARRIER_TYPE_TEXTURE,
        .NumBarriers = count,
        .pTextureBarriers = d3d12_barriers,
    };

    command_list->Barrier(1, &barrier_group);
    return true;
}

void D3D12Context::init_imgui(const DisplayWindow& window, const Swapchain& swapchain)
{
    // Create dedicated heap for ImGUI
    const D3D12_DESCRIPTOR_HEAP_DESC desc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = swapchain.buffer_count,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0,
    };
    m_imgui_heap.create(m_device.Get(), desc);


    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = m_device.Get();
    init_info.CommandQueue = m_graphics_queue.Get();
    init_info.NumFramesInFlight = static_cast<unsigned>(swapchain.buffer_count);
    init_info.RTVFormat = dxgi_format(swapchain.format);
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    init_info.SrvDescriptorHeap = m_imgui_heap.get().Get();

    static struct qinfo
    {
        D3D12DescriptorHeap* heap;
        std::array<Descriptor, 2>* descriptors;
    } info{
        .heap = &m_imgui_heap,
        .descriptors = &m_imgui_descriptors,
    };
    static unsigned descriptor_index = 0;

    init_info.UserData = &info;

    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info,
                                        D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
                                        D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
    {
        const auto qin = static_cast<qinfo*>(info->UserData);
        static UINT index = 0;

        auto& array = *qin->descriptors;

        array[index].offset = descriptor_index++;

        qin->heap->get_CPU_descriptor(out_cpu_handle, array[index].offset);
        qin->heap->get_GPU_descriptor(out_gpu_handle, array[index].offset);

        index++;
    };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info,
                                       D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
                                       D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
    {
        OutputDebugStringA("WARNING: ImGui descriptors not freed\n");
    };
    ImGui_ImplSDL3_InitForD3D(window.get_window());
    ImGui_ImplDX12_Init(&init_info);
}

void D3D12Context::start_imgui_frame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void D3D12Context::render_imgui_draw_data(CommandList* cmd_list)
{
    const auto cmd_list_d3d12 = to_internal(*cmd_list);
    ID3D12DescriptorHeap* heaps[] = {m_imgui_heap.get().Get()};
    cmd_list_d3d12->list.Get()->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list_d3d12->list.Get());
}

void D3D12Context::destroy_imgui()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

bool D3D12Context::compatibility_set_constant_buffers(unsigned slot,
                                                      unsigned count,
                                                      Buffer* const* buffers,
                                                      PipelineStage stage)
{
    // Should not be relying on this in D3D12
    return false;
}

bool D3D12Context::compatibility_set_shader_buffers(unsigned slot,
                                                    unsigned count,
                                                    Descriptor* const* descriptors,
                                                    PipelineStage stage)
{
    // Should not be relying on this in D3D12
    return false;
}

bool D3D12Context::compatibility_set_uav_buffers(unsigned slot, unsigned count, Buffer* const* buffers)
{
    // Should not be relying on this in D3D12
    return false;
}

bool D3D12Context::compatibility_set_textures(
    unsigned slot, unsigned count, Descriptor* const* descriptors, AccessFlags flag, PipelineStage stage)
{
    // Should not be relying on this in D3D12
    return false;
}

bool D3D12Context::compatibility_set_samplers(unsigned slot,
                                              unsigned count,
                                              Descriptor* const* samplers,
                                              PipelineStage stage)
{
    // Should not be relying on this in D3D12
    return false;
}

bool D3D12Context::wait_idle(const QueueType queue)
{
    m_fence_wait_all_last_signaled += 1;
    auto value = m_fence_wait_all_last_signaled;

    const auto fence = to_internal(m_fence_wait_all);

    if (FAILED(get_command_queue(queue)->Signal(fence->fence.Get(), value)))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to signal fence for wait idle\n");
        return false;
    }

    const WaitInfo wait_info{.wait_all = true, .count = 1, .fences = &m_fence_wait_all, .values = &value};
    return wait_fences(wait_info);
}

D3D12Context::~D3D12Context()
{
    m_allocator.Reset();
    m_swapchain.Reset();
    m_dxgi_factory.Reset();
    if (m_debug)
    {
        m_dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
        m_dxgi_debug.Reset();
        m_debug.Reset();
    }
    m_allocator.Reset();
    m_device.Reset();
}
