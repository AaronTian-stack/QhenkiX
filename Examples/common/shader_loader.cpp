#include "example_shared/shader_loader.h"

#include <qhenki/utility/file_util.h>
#include <qhenki/utility/string_util.h>

bool read_compiled_shader_blob(const qhenki::gfx::API api,
                               const char* name,
                               uPtr<std::byte[]>* out_data,
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

    std::byte* raw = nullptr;
    size_t size = 0;
    if (!qhenki::util::read_file(path.buffer.data(), &raw, &size))
    {
        return false;
    }

    out_data->reset(raw);
    *out_size = size;
    return true;
}
