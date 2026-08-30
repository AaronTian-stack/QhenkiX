#pragma once

#include <shader_blob.h>

#include "qhenki/rhi/shader.h"

namespace qhenki::util
{
class ShaderBlob
{
public:
    static bool find_shader(const gfx::Shader& blob,
                            gfx::Shader* const out_shader,
                            const char* const* defines = nullptr,
                            const uint32_t define_count = 0)
    {
        if (!out_shader)
        {
            return false;
        }

        const SXC::ShaderView blob_view{
            .data = blob.data,
            .size = blob.size,
        };
        SXC::ShaderView shader_view{};
        if (!SXC::ShaderBlob::find_shader(blob_view, &shader_view, defines, define_count))
        {
            return false;
        }

        *out_shader = {
            .data = shader_view.data,
            .size = shader_view.size,
        };
        return true;
    }
};
} // namespace qhenki::util
