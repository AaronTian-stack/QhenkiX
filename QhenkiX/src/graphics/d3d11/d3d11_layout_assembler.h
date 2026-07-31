#pragma once

#include <d3d11.h>
#include <d3d11shader.h>
#include <tsl/robin_map.h>
#include <wrl/client.h>
#include <mutex>
#include <optional>

#include "qhenki/rhi/shader.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
struct D3D11Layout
{
    ComPtr<ID3D11InputLayout> layout;
    std::vector<D3D11_INPUT_ELEMENT_DESC> desc;
};

struct InputLayoutResult
{
    ID3D11InputLayout* layout;
    bool is_empty;
};

class D3D11LayoutAssembler
{
    std::mutex m_layout_mutex; // For compiling shaders from multiple threads
    tsl::robin_map<size_t, D3D11Layout> m_layout_map;
    tsl::robin_map<ID3D11InputLayout*, D3D11Layout*> m_layout_logical_map;

public:
    ID3D11InputLayout* find_layout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout);
    D3D11Layout* find_layout(ID3D11InputLayout* layout);

    static std::vector<D3D11_INPUT_ELEMENT_DESC> create_input_layout_desc(ID3D11ShaderReflection* vs_reflection,
                                                                          const D3D11_SHADER_DESC& shader_desc,
                                                                          bool increment_slot,
                                                                          unsigned* out_builtins);

    InputLayoutResult create_input_layout_reflection(ID3D11Device* device,
                                                     Shader shader,
                                                     bool increment_slot,
                                                     const char* debug_name);

    // This exists only to stop debug layer from complaining at shutdown
    void clear_maps();
};
} // namespace qhenki::gfx
