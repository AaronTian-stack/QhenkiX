#include "example_shared/shader_loader.h"

#include <qhenki/utility/file_util.h>
#include <qhenki/utility/string_util.h>

#include <cstdio>
#include <utility>

bool append_shader_extension(const qhenki::gfx::API api,
                             const char* shader_name_no_ext,
                             char* out_name,
                             const size_t out_name_size)
{
    if (!shader_name_no_ext || !out_name || out_name_size == 0)
    {
        return false;
    }

    const char* extension;
    switch (api)
    {
    case qhenki::gfx::API::D3D11:
        extension = "dxbc";
        break;
    case qhenki::gfx::API::Vulkan:
        extension = "spv";
        break;
    case qhenki::gfx::API::D3D12:
    default:
        extension = "dxil";
        break;
    }

    const int written = snprintf(out_name, out_name_size, "%s.%s", shader_name_no_ext, extension);
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

    const char* api_str;
    switch (api)
    {
    case qhenki::gfx::API::D3D11:
        api_str = "dx11";
        break;
    case qhenki::gfx::API::Vulkan:
        api_str = "vulkan";
        break;
    case qhenki::gfx::API::D3D12:
    default:
        api_str = "dx12";
        break;
    }

    const auto path = qhenki::util::format_string("compiled-shaders/%s/%s", api_str, name);

    void* raw = nullptr;
    size_t size = 0;
    if (!qhenki::util::read_file(path.buffer.data(), &raw, &size))
    {
        return false;
    }

    *out_data = uPtr<std::byte, void (*)(void*)>(static_cast<std::byte*>(raw),
                                                 [](void* p)
                                                 {
                                                     delete[] static_cast<char*>(p);
                                                 });
    *out_size = size;
    return true;
}
