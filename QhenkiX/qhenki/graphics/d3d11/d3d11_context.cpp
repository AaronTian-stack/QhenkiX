#include "d3d11_context.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_sdl3.h>
#include "qhenki/utility/string_util.h"

#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXTex.h>

#include "d3d11_heap.h"
#include "d3d11_pipeline.h"
#include "d3d11_shader.h"
#include "d3d11_texture.h"
#include "qhenki/application.h"
#include "qhenki/utility/d3d_util.h"

using namespace qhenki::gfx;

namespace
{
ComPtr<ID3D11Buffer>* to_internal(const Buffer& ext)
{
    const auto d3d11_buffer = static_cast<ComPtr<ID3D11Buffer>*>(ext.internal_state.get());
    assert(d3d11_buffer);
    return d3d11_buffer;
}

D3D11Shader* to_internal(const Shader& ext)
{
    const auto d3d11_shader = static_cast<D3D11Shader*>(ext.internal_state.get());
    assert(d3d11_shader);
    return d3d11_shader;
}

D3D11GraphicsPipeline* to_internal(const GraphicsPipeline& ext)
{
    const auto d3d11_pipeline = static_cast<D3D11GraphicsPipeline*>(ext.internal_state.get());
    assert(d3d11_pipeline);
    return d3d11_pipeline;
}

D3D11Texture* to_internal(const Texture& ext)
{
    const auto d3d11_texture = static_cast<D3D11Texture*>(ext.internal_state.get());
    assert(d3d11_texture);
    return d3d11_texture;
}

D3D11_SRV_UAV_Heap* to_internal_srv_uav(const DescriptorHeap& ext)
{
    const auto d3d11_heap = static_cast<D3D11_SRV_UAV_Heap*>(ext.internal_state.get());
    assert(d3d11_heap);
    return d3d11_heap;
}

D3D11_Sampler_Heap* to_internal_sampler(const DescriptorHeap& ext)
{
    const auto d3d11_heap = static_cast<D3D11_Sampler_Heap*>(ext.internal_state.get());
    assert(d3d11_heap);
    return d3d11_heap;
}

ID3D11Resource* get_texture_resource(const D3D11Texture& tex)
{
    const auto& texture = tex.texture;
    if (std::holds_alternative<ComPtr<ID3D11Texture1D>>(texture))
    {
        return std::get<ComPtr<ID3D11Texture1D>>(texture).Get();
    }
    if (std::holds_alternative<ComPtr<ID3D11Texture2D>>(texture))
    {
        return std::get<ComPtr<ID3D11Texture2D>>(texture).Get();
    }
    if (std::holds_alternative<ComPtr<ID3D11Texture3D>>(texture))
    {
        return std::get<ComPtr<ID3D11Texture3D>>(texture).Get();
    }
    return nullptr;
}
} // namespace

bool qhenki::gfx::set_debug_name(ID3D11DeviceChild* obj, const char* debug_name)
{
    if (obj && debug_name)
    {
        return SUCCEEDED(obj->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(debug_name), debug_name));
    }
    return false;
}

ID3D11DepthStencilView* D3D11Context::start_dsv(const RenderTarget* const depth_stencil) const
{
    ID3D11DepthStencilView* ds = nullptr;
    if (depth_stencil)
    {
        assert(is_depth_stencil_format(depth_stencil->texture->desc.format));
        if (depth_stencil->clear_type != RenderTarget::ClearType::NONE)
        {
            const auto state = to_internal(*depth_stencil->texture);
            if (!state->dsv_view)
            {
                if (FAILED(m_device->CreateDepthStencilView(get_texture_resource(*state),
                                                            nullptr,
                                                            state->dsv_view.ReleaseAndGetAddressOf())))
                {
                    OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Depth Stencil View\n");
                    return nullptr;
                }
            }
            ds = state->dsv_view.Get();

            D3D11_CLEAR_FLAG clear = static_cast<D3D11_CLEAR_FLAG>(0);
            if (depth_stencil->clear_type & RenderTarget::ClearType::DEPTH)
            {
                clear = static_cast<D3D11_CLEAR_FLAG>(static_cast<int>(clear) | static_cast<int>(D3D11_CLEAR_DEPTH));
            }
            if (depth_stencil->clear_type & RenderTarget::ClearType::STENCIL)
            {
                clear = static_cast<D3D11_CLEAR_FLAG>(static_cast<int>(clear) | static_cast<int>(D3D11_CLEAR_STENCIL));
            }
            assert(clear);

            const auto& [clear_depth_value, clear_stencil_value] = depth_stencil->clear_params.dsv_clear_params;
            m_device_context->ClearDepthStencilView(ds, clear, clear_depth_value, clear_stencil_value);
        }
    }
    return ds;
}

std::string D3D11Context::create(const bool enable_debug_layer)
{
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgi_factory))))
    {
        return "D3D11: Failed to create DXGI Factory";
    }

    BOOL allow_tearing = FALSE;
    if (FAILED(m_dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(BOOL))))
    {
        return "D3D11: Failed to check for allow tearing support";
    }
    m_allow_tearing = allow_tearing == TRUE;

    if (enable_debug_layer)
    {
        constexpr char factory_name[] = "DXGI Factory";
        m_dxgi_factory->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(factory_name), factory_name);
    }

    // Pick discrete GPU
    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(m_dxgi_factory->EnumAdapterByGpuPreference(0, // Adapter index
                                                          DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                          __uuidof(IDXGIAdapter1),
                                                          reinterpret_cast<void**>(adapter.GetAddressOf()))))
    {
        return "D3D11: Failed to find a discrete GPU adapter";
    }

    DXGI_ADAPTER_DESC1 desc;
    const auto hr = adapter->GetDesc1(&desc);
    if (SUCCEEDED(hr))
    {
        const auto adapter_description = util::format_wstring(L"Adapter: %s, Vendor ID: 0x%04X, Device ID: 0x%04X\n",
                                                              desc.Description,
                                                              desc.VendorId,
                                                              desc.DeviceId);
        OutputDebugStringW(adapter_description.buffer.data());
    }

    // TODO: Increase to 11_1 for UAV in vertex shader?
    constexpr D3D_FEATURE_LEVEL device_feature_level = D3D_FEATURE_LEVEL_11_0;

    UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (enable_debug_layer)
    {
        creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    if (FAILED(D3D11CreateDevice(adapter.Get(),
                                 D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN,
                                 nullptr,
                                 creation_flags,
                                 &device_feature_level,
                                 1,
                                 D3D11_SDK_VERSION,
                                 &m_device,
                                 nullptr,
                                 &m_device_context)))
    {
        return "D3D11: Failed to create D3D11 Device";
    }

    if (enable_debug_layer)
    {
        constexpr char device_name[] = "d3d11_device";
        m_device->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(device_name), device_name);
        if (FAILED(m_device.As(&m_debug)))
        {
            return "D3D11: Failed to get the debug layer from the device";
        }
    }

    if (FAILED(m_device_context.As(&m_multithread)))
    {
        return "D3D11: Failed to get ID3D10Multithread interface from device context";
    }
    m_multithread->SetMultithreadProtected(TRUE);

    return "";
}

bool D3D11Context::is_compatibility() const
{
    return true;
}

namespace
{
bool create_swapchain_resources(ID3D11Device* device, IDXGISwapChain1* swapchain, ID3D11RenderTargetView** out_rtv)
{
    ComPtr<ID3D11Texture2D> back_buffer;
    if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer))))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to get Back Buffer from Swapchain\n");
        return false;
    }
    if (FAILED(device->CreateRenderTargetView(back_buffer.Get(), nullptr, out_rtv)))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Render Target View\n");
        return false;
    }
    set_debug_name(*out_rtv, "Swapchain Render Target");
    return true;
}
} // namespace

bool D3D11Context::create_swapchain(const DisplayWindow& window,
                                    const SwapchainDesc& swapchain_desc,
                                    Queue* const direct_queue,
                                    unsigned* const frame_index)
{
    if (swapchain_desc.tearing && !m_allow_tearing)
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Tearing is not supported on this system\n");
        return false;
    }

    *frame_index = 0;
    UINT swapchain_flags = 0;
    if (swapchain_desc.tearing && m_allow_tearing)
    {
        swapchain_flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    const DXGI_SWAP_CHAIN_DESC1 dxgi_desc = {
        .Width = static_cast<UINT>(swapchain_desc.width),
        .Height = static_cast<UINT>(swapchain_desc.height),
        .Format = swapchain_desc.format,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = swapchain_desc.buffer_count,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = swapchain_flags,
    };

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {};
    fullscreen_desc.Windowed = true;

    const auto hwnd = window.get_hwnd();
    if (!hwnd ||
        FAILED(m_dxgi_factory->CreateSwapChainForHwnd(
            m_device.Get(), hwnd, &dxgi_desc, &fullscreen_desc, nullptr, m_swapchain.ReleaseAndGetAddressOf())))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Swapchain\n");
        return false;
    }

    if (!create_swapchain_resources(m_device.Get(), m_swapchain.Get(), m_swapchain_view.ReleaseAndGetAddressOf()))
    {
        return false;
    }

    return true;
}

bool D3D11Context::resize_swapchain(Swapchain* const swapchain,
                                    const int width,
                                    const int height,
                                    unsigned& frame_index)
{
    m_device_context->Flush();

    m_swapchain_view.Reset();

    UINT resize_flags = 0;
    if (swapchain->tearing && m_allow_tearing)
    {
        resize_flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }
    if (FAILED(m_swapchain->ResizeBuffers(0, width, height, swapchain->format, resize_flags)))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to resize Swapchain buffers\n");
        return false;
    }
    return create_swapchain_resources(m_device.Get(), m_swapchain.Get(), m_swapchain_view.ReleaseAndGetAddressOf());
}

bool D3D11Context::present(const Swapchain& swapchain,
                           unsigned fence_count,
                           Fence* wait_fences,
                           unsigned swapchain_index)
{
    UINT sync_interval = 1;
    UINT flags = 0;

    if (swapchain.tearing && m_allow_tearing)
    {
        sync_interval = 0;
        flags |= DXGI_PRESENT_ALLOW_TEARING;
    }

    if (SUCCEEDED(m_swapchain->Present(sync_interval, flags)))
    {
        m_frame_index = ++m_frame_index % Application::m_frames_in_flight;
        return true;
    }
    return false;
}

unsigned D3D11Context::get_swapchain_frame_index(const Swapchain& swapchain)
{
    return m_frame_index;
}

bool D3D11Context::create_shader(void* data, size_t size, ShaderType type, Shader* shader)
{
    bool result = true;
    shader->internal_state = mkS<D3D11Shader>(m_device.Get(), type, data, size, nullptr, &result);
    return result;
}

bool D3D11Context::create_pipeline(const GraphicsPipelineDesc& desc,
                                   GraphicsPipeline* pipeline,
                                   const Shader& vertex_shader,
                                   const Shader& pixel_shader,
                                   PipelineLayout* in_layout,
                                   const char* debug_name)
{
    auto d3d11_pipeline = mkS<D3D11GraphicsPipeline>();
    const auto d3d11_vertex_shader = to_internal(vertex_shader);

    d3d11_pipeline->vertex_shader = vertex_shader.internal_state.get();
    d3d11_pipeline->pixel_shader = pixel_shader.internal_state.get();

    const auto true_vs = std::get_if<D3D11VertexShader>(&d3d11_vertex_shader->m_shader);
    assert(true_vs);

    ID3D11InputLayout* input_layout_ = m_layout_assembler.create_input_layout_reflection(
        m_device.Get(), true_vs->vertex_shader_blob.Get(), desc.increment_slot);
    d3d11_pipeline->input_layout = input_layout_;

    bool succeeded = input_layout_ != nullptr;

    const RasterizerDesc rs = desc.rasterizer_state.value_or(RasterizerDesc{});
    D3D11_RASTERIZER_DESC rasterizer_desc = {
        .FillMode = static_cast<D3D11_FILL_MODE>(rs.fill_mode),
        .CullMode = static_cast<D3D11_CULL_MODE>(rs.cull_mode),
        .FrontCounterClockwise = rs.front_counter_clockwise,
        .DepthBias = rs.depth_bias,
        .DepthBiasClamp = rs.depth_bias_clamp,
        .SlopeScaledDepthBias = rs.slope_scaled_depth_bias,
        .DepthClipEnable = rs.depth_clip_enable,
        .ScissorEnable = FALSE,         // Scissor enable not included (TODO: add later?)
        .MultisampleEnable = FALSE,     // Multisample enable not included (TODO: add later?)
        .AntialiasedLineEnable = FALSE, // Antialiased line not included (TODO: add later?)
    };
    if (FAILED(m_device->CreateRasterizerState(&rasterizer_desc,
                                               d3d11_pipeline->rasterizer_state.ReleaseAndGetAddressOf())))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Rasterizer State");
        succeeded = false;
    }

    if (const auto& blend = desc.blend_desc; blend.has_value())
    {
        D3D11_BLEND_DESC blend_desc{
            .AlphaToCoverageEnable = blend->AlphaToCoverageEnable,
            .IndependentBlendEnable = blend->IndependentBlendEnable,
        };
        for (int i = 0; i < 8; i++)
        {
            // D3D11 does not have logic operations
            assert(!(blend->RenderTarget[i].BlendEnable && blend->RenderTarget[i].LogicOpEnable));
            blend_desc.RenderTarget[i] = {
                .BlendEnable = blend->RenderTarget[i].BlendEnable,
                .SrcBlend = static_cast<D3D11_BLEND>(blend->RenderTarget[i].SrcBlend),
                .DestBlend = static_cast<D3D11_BLEND>(blend->RenderTarget[i].DestBlend),
                .BlendOp = static_cast<D3D11_BLEND_OP>(blend->RenderTarget[i].BlendOp),
                .SrcBlendAlpha = static_cast<D3D11_BLEND>(blend->RenderTarget[i].SrcBlendAlpha),
                .DestBlendAlpha = static_cast<D3D11_BLEND>(blend->RenderTarget[i].DestBlendAlpha),
                .BlendOpAlpha = static_cast<D3D11_BLEND_OP>(blend->RenderTarget[i].BlendOpAlpha),
                .RenderTargetWriteMask = blend->RenderTarget[i].RenderTargetWriteMask,
            };
        }
        if (FAILED(m_device->CreateBlendState(&blend_desc, &d3d11_pipeline->blend_state)))
        {
            OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Blend State\n");
            succeeded = false;
        }
    }

    if (const auto& ds = desc.depth_stencil_state; ds.has_value())
    {
        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {
            .DepthEnable = static_cast<BOOL>(ds->depth_enable),
            .DepthWriteMask = static_cast<D3D11_DEPTH_WRITE_MASK>(ds->depth_write_mask),
            .DepthFunc = static_cast<D3D11_COMPARISON_FUNC>(ds->depth_func),
            .StencilEnable = ds->stencil_enable,
            .StencilReadMask = ds->stencil_read_mask,
            .StencilWriteMask = ds->stencil_write_mask,
            .FrontFace = {.StencilFailOp = static_cast<D3D11_STENCIL_OP>(ds->front_face.StencilFailOp),
                          .StencilDepthFailOp = static_cast<D3D11_STENCIL_OP>(ds->front_face.StencilDepthFailOp),
                          .StencilPassOp = static_cast<D3D11_STENCIL_OP>(ds->front_face.StencilPassOp),
                          .StencilFunc = static_cast<D3D11_COMPARISON_FUNC>(ds->front_face.StencilFunc)},
            .BackFace = {.StencilFailOp = static_cast<D3D11_STENCIL_OP>(ds->back_face.StencilFailOp),
                         .StencilDepthFailOp = static_cast<D3D11_STENCIL_OP>(ds->back_face.StencilDepthFailOp),
                         .StencilPassOp = static_cast<D3D11_STENCIL_OP>(ds->back_face.StencilPassOp),
                         .StencilFunc = static_cast<D3D11_COMPARISON_FUNC>(ds->back_face.StencilFunc)},
        };

        if (FAILED(m_device->CreateDepthStencilState(&depth_stencil_desc,
                                                     d3d11_pipeline->depth_stencil_state.ReleaseAndGetAddressOf())))
        {
            OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Depth Stencil State\n");
            succeeded = false;
        }
    }

    if (succeeded)
    {
        pipeline->internal_state = std::move(d3d11_pipeline);
    }
    return succeeded;
}

bool D3D11Context::bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline)
{
    const auto d3d11_pipeline = to_internal(pipeline);
    d3d11_pipeline->bind(m_device_context.Get());
    return true;
}

bool D3D11Context::create_pipeline_layout(PipelineLayoutDesc* const desc, PipelineLayout* layout)
{
    return true;
}

void D3D11Context::bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout)
{
}

bool D3D11Context::set_pipeline_constant(
    CommandList* cmd_list, unsigned param, uint32_t offset, unsigned size, void* data)
{
    // Return false instead of true like in other no ops
    // You shouldn't be calling this path in compatibility context
    return false;
}

bool D3D11Context::create_descriptor_heap(const DescriptorHeapDesc& desc,
                                          DescriptorHeap* const heap,
                                          const char* debug_name)
{
    switch (desc.type)
    {
    case DescriptorHeapDesc::Type::CBV_SRV_UAV:
        heap->internal_state = mkS<D3D11_SRV_UAV_Heap>();
        break;
    case DescriptorHeapDesc::Type::SAMPLER:
        heap->internal_state = mkS<D3D11_Sampler_Heap>();
        break;
    default:
        OutputDebugStringA("Qhenki D3D11 ERROR: Unsupported descriptor heap type\n");
        return false;
    }

    // TODO: Force fixed size for slightly stricter match with D3D12?
    return true;
}

size_t D3D11Context::get_descriptor_heap_max_size(DescriptorHeapDesc::Type type)
{
    return 0;
}

void D3D11Context::set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap)
{
}

void D3D11Context::set_descriptor_heap(CommandList* cmd_list,
                                       const DescriptorHeap& heap,
                                       const DescriptorHeap& sampler_heap)
{
}

void D3D11Context::set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor)
{
}

bool D3D11Context::copy_descriptors(size_t bytes, const Descriptor& src, const Descriptor& dst)
{
    return true;
}

bool D3D11Context::free_descriptor(Descriptor* descriptor)
{
    return true;
}

size_t D3D11Context::get_descriptor_size(Descriptor::Type type) const
{
    return 0;
}

size_t D3D11Context::get_descriptor_alignment(Descriptor::Type type) const
{
    return 0;
}

bool D3D11Context::create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, const char* debug_name)
{
    assert(buffer);
    if (desc.usage & BufferUsage::CONSTANT)
    {
        if (desc.size > D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16)
        {
            OutputDebugStringA("Qhenki D3D11 ERROR: Buffer size exceeds maximum constant buffer size\n");
            return false;
        }
        if (desc.size % 16 != 0)
        {
            OutputDebugStringA("Qhenki D3D11 ERROR: Constant buffer size is not aligned to 16 bytes\n");
            return false;
        }
    }

    assert(desc.size <= UINT_MAX);
    assert(desc.stride <= UINT_MAX);

    D3D11_BUFFER_DESC buffer_info{
        .ByteWidth = static_cast<UINT>(desc.size),
        .StructureByteStride = static_cast<UINT>(desc.stride),
    };

    if (desc.usage & BufferUsage::VERTEX)
    {
        buffer_info.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
    }
    if (desc.usage & BufferUsage::INDEX)
    {
        buffer_info.BindFlags |= D3D11_BIND_INDEX_BUFFER;
    }
    if (desc.usage & BufferUsage::CONSTANT)
    {
        buffer_info.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
    }
    if (desc.usage & BufferUsage::SHADER)
    {
        buffer_info.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        buffer_info.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    }
    if (desc.usage & BufferUsage::UAV)
    {
        buffer_info.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }
    if (desc.usage & BufferUsage::INDIRECT)
    {
        buffer_info.BindFlags |= D3D11_BIND_UNORDERED_ACCESS; // TODO: check this
    }
    if (desc.visibility & CPU_SEQUENTIAL)
    {
        buffer_info.Usage = D3D11_USAGE_DYNAMIC;
        buffer_info.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        buffer_info.Usage = D3D11_USAGE_DEFAULT;
        buffer_info.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA resource_data;
    resource_data.pSysMem = data;
    const auto resource_data_ptr = data && desc.visibility == CPU_SEQUENTIAL ? &resource_data : nullptr;

    ComPtr<ID3D11Buffer> d3d11_buffer;
    if (FAILED(m_device->CreateBuffer(&buffer_info, resource_data_ptr, d3d11_buffer.ReleaseAndGetAddressOf())))
    {
        return false;
    }

    if (m_debug)
    {
        set_debug_name(d3d11_buffer.Get(), debug_name);
    }

    buffer->desc = desc;
    buffer->internal_state = mkS<ComPtr<ID3D11Buffer>>(std::move(d3d11_buffer));
    return true;
}

bool D3D11Context::create_descriptor_constant_view(const Buffer& buffer,
                                                   DescriptorHeap* const heap,
                                                   Descriptor* descriptor)
{
    // D3D11 constant buffers don't need views
    return true;
}

bool D3D11Context::create_descriptor_shader_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor)
{
    assert(heap);
    assert(descriptor);
    const auto buffer_d3d11 = to_internal(buffer);
    const auto heap_d3d11 = to_internal_srv_uav(*heap);

    const auto create_new_descriptor = descriptor->offset == CREATE_NEW_DESCRIPTOR;

    const auto offset = create_new_descriptor ? heap_d3d11->shader_resource_views.size() : descriptor->offset;

    if (create_new_descriptor)
    {
        heap_d3d11->shader_resource_views.push_back({});
    }

    ComPtr<ID3D11ShaderResourceView> view;
    const D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
        .Buffer =
            {
                .FirstElement = 0,
                .NumElements = static_cast<UINT>(buffer.desc.size / buffer.desc.stride),
            },
    };
    if (FAILED(m_device->CreateShaderResourceView(buffer_d3d11->Get(), &srv_desc, view.ReleaseAndGetAddressOf())))
    {
        if (create_new_descriptor)
        {
            heap_d3d11->shader_resource_views.pop_back();
        }
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create buffer SRV\n");
        return false;
    }
    heap_d3d11->shader_resource_views[offset] = std::move(view);

    *descriptor = Descriptor(heap, offset);

    return true;
}

void D3D11Context::copy_buffer(CommandList* cmd_list,
                               const Buffer& src,
                               const uint64_t src_offset,
                               Buffer* dst,
                               const uint64_t dst_offset,
                               const uint64_t bytes)
{
    assert(dst);
    assert(src_offset + bytes <= src.desc.size);
    assert(dst_offset + bytes <= dst->desc.size);

    const auto src_d3d11 = to_internal(src);
    const auto dst_d3d11 = to_internal(*dst);

    // Assume 1D for now
    const auto box = CD3D11_BOX(static_cast<long>(src_offset), 0, 0, static_cast<long>(src_offset + bytes), 1, 1);

    // Copy entire buffer for now
    // TODO: per subresource
    m_device_context->CopySubresourceRegion(dst_d3d11->Get(),
                                            0, // Dst subresource
                                            static_cast<long>(dst_offset),
                                            0,
                                            0,
                                            src_d3d11->Get(),
                                            0, // Src subresource
                                            &box);
}

bool D3D11Context::create_texture(const TextureDesc& desc, Texture* texture, const char* debug_name)
{
    assert(texture);
    auto texture_d3d11 = mkS<D3D11Texture>();

    UINT bind_flags = D3D11_BIND_SHADER_RESOURCE;
    if (is_depth_stencil_format(desc.format))
    {
        bind_flags = D3D11_BIND_DEPTH_STENCIL; // Overwrite
    }
    if (desc.is_render_target)
    {
        bind_flags |= D3D11_BIND_RENDER_TARGET;
    }

    if (desc.dimension == TextureDimension::TEXTURE_1D)
    {
        const D3D11_TEXTURE1D_DESC texture_desc{
            .Width = static_cast<UINT>(desc.width),
            .MipLevels = desc.mip_levels,
            .ArraySize = desc.depth_or_array_size,
            .Format = desc.format,
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = bind_flags,
            .CPUAccessFlags = 0,
            .MiscFlags = 0,
        };

        texture_d3d11->texture.emplace<ComPtr<ID3D11Texture1D>>();
        if (FAILED(m_device->CreateTexture1D(
                &texture_desc,
                nullptr,
                std::get<ComPtr<ID3D11Texture1D>>(texture_d3d11->texture).ReleaseAndGetAddressOf())))
        {
            OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create 1D texture\n");
            return false;
        }
    }
    else if (desc.dimension == TextureDimension::TEXTURE_2D)
    {
        const D3D11_TEXTURE2D_DESC texture_desc{
            .Width = static_cast<UINT>(desc.width),
            .Height = static_cast<UINT>(desc.height),
            .MipLevels = desc.mip_levels,
            .ArraySize = desc.depth_or_array_size,
            .Format = desc.format,
            .SampleDesc = {.Count = desc.sample_count, .Quality = 0},
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = bind_flags,
            .CPUAccessFlags = 0,
            .MiscFlags = 0, // TODO: cubemaps?
        };

        texture_d3d11->texture.emplace<ComPtr<ID3D11Texture2D>>();
        if (FAILED(m_device->CreateTexture2D(
                &texture_desc,
                nullptr,
                std::get<ComPtr<ID3D11Texture2D>>(texture_d3d11->texture).ReleaseAndGetAddressOf())))
        {
            OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create 2D texture\n");
            return false;
        }
    }
    else if (desc.dimension == TextureDimension::TEXTURE_3D)
    {
        const D3D11_TEXTURE3D_DESC texture_desc{
            .Width = static_cast<UINT>(desc.width),
            .Height = static_cast<UINT>(desc.height),
            .Depth = static_cast<UINT>(desc.depth_or_array_size),
            .MipLevels = desc.mip_levels,
            .Format = desc.format,
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = bind_flags,
            .CPUAccessFlags = 0,
            .MiscFlags = 0,
        };

        texture_d3d11->texture.emplace<ComPtr<ID3D11Texture3D>>();
        if (FAILED(m_device->CreateTexture3D(
                &texture_desc,
                nullptr,
                std::get<ComPtr<ID3D11Texture3D>>(texture_d3d11->texture).ReleaseAndGetAddressOf())))
        {
            OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create 3D texture\n");
            return false;
        }
    }

    texture->desc = desc;
    texture->internal_state = std::move(texture_d3d11);
    return true;
}

bool D3D11Context::create_descriptor_shader_view(const Texture& texture,
                                                 DescriptorHeap* const heap,
                                                 Descriptor* const descriptor)
{
    assert(descriptor);
    const auto texture_d3d11 = to_internal(texture);
    const auto heap_d3d11 = to_internal_srv_uav(*heap);

    const auto create_new_descriptor = descriptor->offset == CREATE_NEW_DESCRIPTOR;

    const size_t offset = create_new_descriptor ? heap_d3d11->shader_resource_views.size() : descriptor->offset;
    if (create_new_descriptor)
    {
        heap_d3d11->shader_resource_views.push_back({});
    }

    const auto resource = get_texture_resource(*texture_d3d11);
    assert(resource);

    ComPtr<ID3D11ShaderResourceView> view;
    if (FAILED(m_device->CreateShaderResourceView(resource, nullptr, view.ReleaseAndGetAddressOf())))
    {
        if (create_new_descriptor)
        {
            heap_d3d11->shader_resource_views.pop_back();
        }
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create texture SRV\n");
        return false;
    }
    heap_d3d11->shader_resource_views[offset] = std::move(view);

    *descriptor = Descriptor(heap, offset);

    return true;
}

bool D3D11Context::copy_to_texture(CommandList* cmd_list,
                                   const void* data,
                                   Buffer* const staging,
                                   Texture* const texture)
{
    assert(staging);
    const auto texture_d3d11 = to_internal(*texture);
    ID3D11Resource* resource = get_texture_resource(*texture_d3d11);
    if (!resource)
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to get texture resource\n");
        return false;
    }

    const UINT32 num_subresources = texture->desc.mip_levels * texture->desc.depth_or_array_size;
    size_t data_offset = 0;
    for (UINT32 subresource = 0; subresource < num_subresources; subresource++)
    {
        const UINT32 mip = subresource % texture->desc.mip_levels;

        const UINT32 mip_width = std::max(1u, static_cast<UINT32>(texture->desc.width) >> mip);
        const UINT32 mip_height = std::max(1u, texture->desc.height >> mip);
        const UINT32 mip_depth = texture->desc.dimension == TextureDimension::TEXTURE_3D
                                   ? std::max<UINT32>(1u, texture->desc.depth_or_array_size >> mip)
                                   : 1u;

        size_t row_pitch = 0;
        size_t slice_pitch = 0;
        if (FAILED(ComputePitch(texture->desc.format, mip_width, mip_height, row_pitch, slice_pitch)))
        {
            // If failed texture is partially updated but this branch shouldn't happen
            return false;
        }

        const UINT8* src = static_cast<const UINT8*>(data) + data_offset;
        m_device_context->UpdateSubresource(
            resource, subresource, nullptr, src, static_cast<UINT>(row_pitch), static_cast<UINT>(slice_pitch));

        data_offset += slice_pitch * mip_depth;
    }

    return true;
}

bool D3D11Context::create_descriptor(const SamplerDesc& desc, DescriptorHeap* const heap, Descriptor* const descriptor)
{
    const D3D11_SAMPLER_DESC sampler_desc{
        .Filter = static_cast<D3D11_FILTER>(filter(desc.min_filter,
                                                   desc.mag_filter,
                                                   desc.mip_filter,
                                                   desc.comparison_func,
                                                   desc.max_anisotropy)), // Shared type values D3D12
        .AddressU = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(
            texture_address_mode(desc.address_mode_u)), // Same in D3D11, D3D12
        .AddressV = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(texture_address_mode(desc.address_mode_v)),
        .AddressW = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(texture_address_mode(desc.address_mode_w)),
        .MipLODBias = desc.mip_lod_bias,
        .MaxAnisotropy = desc.max_anisotropy,
        .ComparisonFunc = desc.comparison_func == ComparisonFunc::NONE
                            ? D3D11_COMPARISON_NEVER
                            : static_cast<D3D11_COMPARISON_FUNC>(
                                  comparison_func(desc.comparison_func)), // D3D11 doesn't have NONE
        .BorderColor = {desc.border_color[0], desc.border_color[1], desc.border_color[2], desc.border_color[3]},
        .MinLOD = desc.min_lod,
        .MaxLOD = desc.max_lod,
    };

    ComPtr<ID3D11SamplerState> sampler_state;
    if (FAILED(m_device->CreateSamplerState(&sampler_desc, sampler_state.ReleaseAndGetAddressOf())))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create sampler state\n");
        return false;
    }

    auto d3d11_heap = to_internal_sampler(*heap);
    d3d11_heap->push_back(std::move(sampler_state));

    *descriptor = Descriptor(heap, d3d11_heap->size() - 1);

    return true;
}

void* D3D11Context::map_buffer(const Buffer& buffer)
{
    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    const auto buffer_d3d11 = to_internal(buffer);
    auto lock = acquire_lock();
    if (FAILED(m_device_context->Map(buffer_d3d11->Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource)))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to map buffer\n");
        return nullptr;
    }
    return mapped_resource.pData;
}

void D3D11Context::unmap_buffer(const Buffer& buffer)
{
    const auto buffer_d3d11 = to_internal(buffer);
    auto lock = acquire_lock();
    m_device_context->Unmap(buffer_d3d11->Get(), 0);
}

void D3D11Context::bind_vertex_buffers(CommandList* cmd_list,
                                       const unsigned start_slot,
                                       const unsigned buffer_count,
                                       const Buffer* const* buffers,
                                       const unsigned* sizes,
                                       const unsigned* const strides,
                                       const unsigned* const offsets)
{
    assert(buffer_count <= D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
    std::array<ID3D11Buffer*, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> buffer_d3d11{};
    for (unsigned int i = 0; i < buffer_count; i++)
    {
        const auto buffer = to_internal(*buffers[i]);
        buffer_d3d11[i] = buffer->Get();
    }
    m_device_context->IASetVertexBuffers(start_slot, buffer_count, buffer_d3d11.data(), strides, offsets);
}

void D3D11Context::bind_index_buffer(CommandList* cmd_list,
                                     const Buffer& buffer,
                                     const IndexType format,
                                     const unsigned offset)
{
    const auto buffer_d3d11 = to_internal(buffer);
    m_device_context->IASetIndexBuffer(buffer_d3d11->Get(), get_dxgi_format(format), offset);
}

bool D3D11Context::create_queue(const QueueType type, Queue* queue)
{
    return true;
}

bool D3D11Context::create_command_pool(CommandPool* command_pool, const Queue& queue)
{
    return true;
}

bool D3D11Context::create_command_list(CommandList* cmd_list, const CommandPool& command_pool, const char* debug_name)
{
    return true;
}

bool D3D11Context::reset_command_list(CommandList* cmd_list, const CommandPool& command_pool)
{
    enter_recording();
    return true;
}


bool D3D11Context::close_command_list(CommandList* cmd_list)
{
    leave_recording();
    return true;
}

bool D3D11Context::reset_command_pool(CommandPool* command_pool)
{
    return true;
}

namespace
{
void unbind_srvs_for_render_targets(ID3D11DeviceContext* ctx)
{
    const std::array<ID3D11ShaderResourceView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> null_srvs{};
    ctx->PSSetShaderResources(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, null_srvs.data());
    ctx->VSSetShaderResources(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, null_srvs.data());
}
} // namespace

bool D3D11Context::start_render_pass(CommandList* cmd_list,
                                     const float* clear_color_values,
                                     const RenderTarget* const depth_stencil,
                                     unsigned frame_index)
{
    unbind_srvs_for_render_targets(m_device_context.Get());
    const auto view = m_swapchain_view.Get();
    m_device_context->ClearRenderTargetView(view, clear_color_values);
    ID3D11DepthStencilView* ds = start_dsv(depth_stencil);
    m_device_context->OMSetRenderTargets(1, &view, ds);
    return true;
}

bool D3D11Context::start_render_pass(CommandList* cmd_list,
                                     const unsigned rt_count,
                                     const RenderTarget* const* rts,
                                     const RenderTarget* const depth_stencil)
{
    std::array<ID3D11RenderTargetView* const*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> rtvs{};
    if (rt_count > rtvs.size())
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Too many render targets");
        return false;
    }
    // Clear render target views (if applicable)
    for (unsigned int i = 0; i < rt_count; i++)
    {
        assert(rts[i]);

        const auto state = to_internal(*rts[i]->texture);

        if (!state->rtv_view)
        {
            if (FAILED(m_device->CreateRenderTargetView(get_texture_resource(*state),
                                                        nullptr,
                                                        state->rtv_view.ReleaseAndGetAddressOf())))
            {
                OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create RTV for render target\n");
                return false;
            }
        }
        if (rts[i]->clear_type & RenderTarget::ClearType::COLOR)
        {
            m_device_context->ClearRenderTargetView(state->rtv_view.Get(),
                                                    rts[i]->clear_params.clear_color_value.data());
        }

        rtvs[i] = state->rtv_view.GetAddressOf();
    }
    ID3D11DepthStencilView* ds = start_dsv(depth_stencil);
    unbind_srvs_for_render_targets(m_device_context.Get());
    m_device_context->OMSetRenderTargets(rt_count, rtvs[0], ds);
    return true;
}

void D3D11Context::set_viewports(CommandList* list, const unsigned count, const D3D12_VIEWPORT* viewport)
{
    for (unsigned int i = 0; i < count; i++)
    {
        m_viewports[i] = {
            .TopLeftX = viewport[i].TopLeftX,
            .TopLeftY = viewport[i].TopLeftY,
            .Width = viewport[i].Width,
            .Height = viewport[i].Height,
            .MinDepth = viewport[i].MinDepth,
            .MaxDepth = viewport[i].MaxDepth,
        };
    }
    m_device_context->RSSetViewports(count, m_viewports.data());
}

void D3D11Context::set_scissor_rects(CommandList* list, const unsigned count, const D3D12_RECT* scissor_rect)
{
    // D3D12_RECT = D3D11_RECT = RECT
    m_device_context->RSSetScissorRects(count, scissor_rect);
}

void D3D11Context::draw(CommandList* cmd_list, const uint32_t vertex_count, const uint32_t start_vertex_offset)
{
    m_device_context->Draw(vertex_count, start_vertex_offset);
}

void D3D11Context::draw_indexed(CommandList* cmd_list,
                                const uint32_t index_count,
                                const uint32_t instance_count,
                                const uint32_t start_index_offset,
                                const int32_t base_vertex_offset,
                                const uint32_t instance_offset)
{
    m_device_context->DrawIndexedInstanced(
        index_count, instance_count, start_index_offset, base_vertex_offset, instance_offset);
}

void D3D11Context::submit_command_lists(const SubmitInfo& submit_info, Queue* queue)
{
}

bool D3D11Context::create_fence(Fence* fence, uint64_t initial_value)
{
    return true;
}

uint64_t D3D11Context::get_fence_value(const Fence& fence)
{
    return 0;
}

bool D3D11Context::wait_fences(const WaitInfo& info)
{
    return true;
}

void D3D11Context::set_barrier_resource(unsigned count,
                                        ImageBarrier* barriers,
                                        const Swapchain& swapchain,
                                        unsigned frame_index)
{
}

void D3D11Context::set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target)
{
}

void D3D11Context::issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers)
{
}

void D3D11Context::init_imgui(const DisplayWindow& window, const Swapchain& swapchain)
{
    auto lock = acquire_lock();
    ImGui_ImplSDL3_InitForD3D(window.get_window());
    ImGui_ImplDX11_Init(m_device.Get(), m_device_context.Get());
}

void D3D11Context::start_imgui_frame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void D3D11Context::render_imgui_draw_data(CommandList* cmd_list)
{
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void D3D11Context::destroy_imgui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

bool D3D11Context::compatibility_set_constant_buffers(const unsigned slot,
                                                      const unsigned count,
                                                      Buffer* const* buffers,
                                                      const PipelineStage stage)
{
    std::array<ID3D11Buffer*, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> buffer_d3d11{};
    if (count > buffer_d3d11.size())
    {
        return false;
    }
    for (unsigned i = 0; i < count; i++)
    {
        buffer_d3d11[i] = to_internal(*buffers[i])->Get();
    }
    switch (stage)
    {
    case PipelineStage::VERTEX:
        m_device_context->VSSetConstantBuffers(slot, count, buffer_d3d11.data());
        break;
    case PipelineStage::PIXEL:
        m_device_context->PSSetConstantBuffers(slot, count, buffer_d3d11.data());
        break;
    case PipelineStage::COMPUTE:
        m_device_context->CSSetConstantBuffers(slot, count, buffer_d3d11.data());
        break;
    default:
        return false;
    }
    return true;
}

bool D3D11Context::compatibility_set_shader_buffers(unsigned slot,
                                                    unsigned count,
                                                    Descriptor* const* descriptors,
                                                    PipelineStage stage)
{
    std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> srv{};
    if (count > srv.size())
    {
        return false;
    }
    assert(*descriptors);
    for (unsigned i = 0; i < count; i++)
    {
        assert(descriptors[i]->heap);
        const auto heap = to_internal_srv_uav(*descriptors[i]->heap);
        srv[i] = heap->shader_resource_views[descriptors[i]->offset].Get();
    }
    switch (stage)
    {
    case PipelineStage::VERTEX:
        m_device_context->VSSetShaderResources(slot, count, srv.data());
        break;
    case PipelineStage::PIXEL:
        m_device_context->PSSetShaderResources(slot, count, srv.data());
        break;
    case PipelineStage::COMPUTE:
        m_device_context->CSSetShaderResources(slot, count, srv.data());
        break;
    }
    return true;
}

bool D3D11Context::compatibility_set_uav_buffers(unsigned slot, unsigned count, Buffer* const* buffers)
{
    // TODO
    assert(false);
    return false;
}

bool D3D11Context::compatibility_set_textures(const unsigned slot,
                                              const unsigned count,
                                              Descriptor* const* descriptors,
                                              const AccessFlags flag,
                                              const PipelineStage stage)
{
    // Read or write (as UAV not RT) access
    union ResourceViews
    {
        std::array<ID3D11UnorderedAccessView*, D3D11_PS_CS_UAV_REGISTER_COUNT> unordered_access_views;
        std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> shader_resource_views;
    } resource_views;

    switch (flag)
    {
    case ACCESS_STORAGE_ACCESS:
        resource_views.unordered_access_views =
            std::array<ID3D11UnorderedAccessView*, D3D11_PS_CS_UAV_REGISTER_COUNT>{};
        break;
    case ACCESS_SHADER_RESOURCE:
        resource_views.shader_resource_views =
            std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>{};
        break;
    default:
        OutputDebugStringA("Qhenki D3D11 ERROR: Invalid access flag for texture\n");
        return false;
    }

    assert(*descriptors);
    for (unsigned i = 0; i < count; i++)
    {
        assert(descriptors[i]->heap);
        const auto heap = to_internal_srv_uav(*descriptors[i]->heap);
        // The descriptor offset is used as index into vector
        switch (flag)
        {
        case ACCESS_STORAGE_ACCESS:
            resource_views.unordered_access_views[i] = heap->unordered_access_views[descriptors[i]->offset].Get();
            break;
        case ACCESS_SHADER_RESOURCE:
            resource_views.shader_resource_views[i] = heap->shader_resource_views[descriptors[i]->offset].Get();
            break;
        default:
            OutputDebugStringA("Qhenki D3D11 ERROR: Invalid access flag for texture\n");
            return false;
        }
    }
    constexpr UINT n1 = ~0u; // Keep current offset
    switch (flag)
    {
    // case ACCESS_RENDER_TARGET:
    case ACCESS_STORAGE_ACCESS:
        switch (stage)
        {
        // TODO: need a better way of doing this
        // D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL and D3D11_KEEP_UNORDERED_ACCESS_VIEWS ?
        case PipelineStage::VERTEX:
        case PipelineStage::PIXEL:
            m_device_context->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
                                                                        nullptr,
                                                                        nullptr,
                                                                        slot,
                                                                        count,
                                                                        resource_views.unordered_access_views.data(),
                                                                        &n1);
            break;
        case PipelineStage::COMPUTE:
            m_device_context->CSSetUnorderedAccessViews(slot,
                                                        count,
                                                        resource_views.unordered_access_views.data(),
                                                        nullptr);
            break;
        default:
            OutputDebugStringA("Qhenki D3D11 ERROR: Invalid pipeline stage for storage access\n");
            return false;
        }
        break;
    case ACCESS_SHADER_RESOURCE:
        switch (stage)
        {
        case PipelineStage::VERTEX:
            m_device_context->VSSetShaderResources(slot, count, resource_views.shader_resource_views.data());
            break;
        case PipelineStage::PIXEL:
            m_device_context->PSSetShaderResources(slot, count, resource_views.shader_resource_views.data());
            break;
        case PipelineStage::COMPUTE:
            m_device_context->CSSetShaderResources(slot, count, resource_views.shader_resource_views.data());
            break;
        }
        break;
    default:
        OutputDebugStringA("Qhenki D3D11 ERROR: Invalid access flag for texture\n");
        return false;
    }
    return true;
}

bool D3D11Context::compatibility_set_samplers(const unsigned slot,
                                              const unsigned count,
                                              Descriptor* const* samplers,
                                              const PipelineStage stage)
{
    std::array<ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_REGISTER_COUNT> sampler_d3d11{};
    if (count > sampler_d3d11.size())
    {
        return false;
    }
    for (unsigned i = 0; i < count; i++)
    {
        if (samplers)
        {
            sampler_d3d11[i] = samplers[i] ? to_internal_sampler(*samplers[i]->heap)->at(i).Get() : nullptr;
        }
        else
        {
            sampler_d3d11[i] = nullptr;
        }
    }
    switch (stage)
    {
    case PipelineStage::VERTEX:
        m_device_context->VSSetSamplers(slot, count, sampler_d3d11.data());
        break;
    case PipelineStage::PIXEL:
        m_device_context->PSSetSamplers(slot, count, sampler_d3d11.data());
        break;
    case PipelineStage::COMPUTE:
        m_device_context->CSSetSamplers(slot, count, sampler_d3d11.data());
        break;
    default:
        return false;
    }
    return true;
}

bool D3D11Context::wait_idle(Queue* const queue)
{
    auto lock = acquire_lock();
    m_device_context->Flush();
    return true;
}

D3D11Context::~D3D11Context()
{
    if (m_device_context)
    {
        m_device_context->ClearState();
        m_device_context->Flush();
    }
    m_multithread.Reset();
    m_device_context.Reset();
    m_dxgi_factory.Reset();
    m_layout_assembler.clear_maps();
    if (m_debug)
    {
        m_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
        m_debug.Reset();
    }
    m_device.Reset();
}
