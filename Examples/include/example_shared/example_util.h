#pragma once

#include <qhenki/application.h>
#include <qhenki/utility/string_util.h>

struct WindowInitPayload
{
    bool fullscreen = false;
};

inline const char* graphics_api_label(const qhenki::gfx::API api)
{
    switch (api)
    {
    case qhenki::gfx::API::D3D11:
        return "DX11";
    case qhenki::gfx::API::D3D12:
        return "DX12";
    case qhenki::gfx::API::Vulkan:
        return "Vulkan";
    default:
        return "undefined";
    }
}

inline qhenki::util::FormatResult<128> make_shader_filename(const qhenki::gfx::API api, const char* basename)
{
    const char* extension;
    switch (api)
    {
    case qhenki::gfx::API::D3D11:
        extension = ".dxbc_blob";
        break;
    case qhenki::gfx::API::D3D12:
        extension = ".dxil_blob";
        break;
    case qhenki::gfx::API::Vulkan:
        extension = ".spv_blob";
        break;
    default:
        return {};
    }

    return qhenki::util::format_string<128>("%s%s", basename, extension);
}

inline void init_display_window_with_name(qhenki::Application& app,
                                          qhenki::DisplayWindow& window,
                                          const char* title,
                                          void* payload)
{
    const auto* p = static_cast<const WindowInitPayload*>(payload);
    const bool fullscreen = p ? p->fullscreen : false;

    const auto window_title = qhenki::util::format_string("%s | %s", title, graphics_api_label(app.get_graphics_api()));

    const qhenki::DisplayInfo info{
        .width = 1280,
        .height = 720,
        .fullscreen = fullscreen,
        .undecorated = false,
        .resizable = true,
        .title = window_title.buffer.data(),
    };

    window.create_window(info, 0);
}
