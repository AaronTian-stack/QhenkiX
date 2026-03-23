#include "example_shared/shader_loader.h"

#include <qhenki/utility/file_util.h>
#include <qhenki/utility/string_util.h>

#include <cstdio>
#include <utility>

const char* get_shader_subdir(const qhenki::gfx::API api)
{
    switch (api)
    {
    case qhenki::gfx::API::D3D11:
        return "dx11";
    case qhenki::gfx::API::Vulkan:
        return "vulkan";
    case qhenki::gfx::API::D3D12:
    default:
        return "dx12";
    }
}

namespace
{
const char* get_shader_extension(const qhenki::gfx::API api)
{
    switch (api)
    {
    case qhenki::gfx::API::D3D11:
        return "dxbc";
    case qhenki::gfx::API::Vulkan:
        return "spv";
    case qhenki::gfx::API::D3D12:
    default:
        return "dxil";
    }
}
} // namespace

bool append_shader_extension(const qhenki::gfx::API api,
                             const char* shader_name_no_ext,
                             char* out_name,
                             const size_t out_name_size)
{
    if (!shader_name_no_ext || !out_name || out_name_size == 0)
    {
        return false;
    }

    const int written = std::snprintf(out_name, out_name_size, "%s.%s", shader_name_no_ext, get_shader_extension(api));
    return written > 0 && std::cmp_less(written, out_name_size);
}

bool read_compiled_shader_bytes(const qhenki::gfx::API api,
                                const char* name,
                                uPtr<std::byte, void (*)(void*)>* out_data,
                                size_t* out_size)
{
    if (!name || !out_data || !out_size)
    {
        return false;
    }

    const auto path = qhenki::util::format_string("compiled-shaders/%s/%s", get_shader_subdir(api), name);

    void* raw = nullptr;
    size_t size = 0;
    if (!qhenki::util::read_file(path.buffer.data(), &raw, &size))
    {
        return false;
    }

    *out_data = uPtr<std::byte, void (*)(void*)>(static_cast<std::byte*>(raw), free);
    *out_size = size;
    return true;
}
