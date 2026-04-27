#include "qhenki/display_window.h"

#include "qhenki/utility/string_util.h"

using namespace qhenki;

void DisplayWindow::create_window(const DisplayInfo& info, const int monitor_index)
{
    create_window_internal(info, monitor_index);
}

SDL_DisplayMode DisplayWindow::get_current_monitor() const
{
    return m_current_monitor;
}

std::vector<SDL_DisplayMode> DisplayWindow::get_monitors()
{
    int display_count;
    const auto displays = SDL_GetDisplays(&display_count);
    std::vector<SDL_DisplayMode> modes;
    modes.reserve(display_count);
    for (int i = 0; i < display_count; i++)
    {
        modes.push_back(*SDL_GetCurrentDisplayMode(displays[i]));
    }
    return modes;
}

XMUINT2 DisplayWindow::get_display_size() const
{
    return {static_cast<uint32_t>(m_display_info.width), static_cast<uint32_t>(m_display_info.height)};
}

const DisplayInfo& DisplayWindow::get_display_info() const
{
    return m_display_info;
}

SDL_Window* DisplayWindow::get_window() const
{
    return m_window;
}

bool DisplayWindow::set_fullscreen(const bool fullscreen)
{
    if (SDL_SetWindowFullscreen(m_window, fullscreen) != 0)
    {
        SDL_Log("Unable to set fullscreen: %s", SDL_GetError());
        return false;
    }
    m_display_info.fullscreen = fullscreen;
    return true;
}

bool DisplayWindow::set_resolution(const int width, const int height)
{
    SDL_SetWindowSize(m_window, width, height);
    m_display_info.width = width;
    m_display_info.height = height;
    return true;
}

void DisplayWindow::set_title(const char* title)
{
    SDL_SetWindowTitle(m_window, title);
    m_display_info.title = title;
}

void DisplayWindow::set_decoration(const bool undecorated)
{
    SDL_SetWindowBordered(m_window, undecorated);
    m_display_info.undecorated = undecorated;
}

void DisplayWindow::set_resizable(const bool resizable)
{
    SDL_SetWindowResizable(m_window, resizable);
    m_display_info.resizable = resizable;
}

DisplayWindow::~DisplayWindow()
{
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void DisplayWindow::create_window_internal(const DisplayInfo& info, int monitor_index)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        const auto str = util::format_string("Unable to initialize SDL: %s", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", str.buffer.data(), nullptr);
        SDL_Quit();
        return;
    }

    // TODO: Choose monitor to create window on

    auto properties_id = SDL_CreateProperties();

    SDL_SetBooleanProperty(properties_id, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);

    SDL_SetBooleanProperty(properties_id, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, info.fullscreen);

    SDL_SetNumberProperty(properties_id, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, info.width);
    SDL_SetNumberProperty(properties_id, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, info.height);

    SDL_SetNumberProperty(properties_id, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(properties_id, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);

    SDL_SetBooleanProperty(properties_id, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, info.undecorated);
    SDL_SetBooleanProperty(properties_id, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, info.resizable);

    SDL_SetStringProperty(properties_id, SDL_PROP_WINDOW_CREATE_TITLE_STRING, info.title);

    m_window = SDL_CreateWindowWithProperties(properties_id);

    m_display_info = info;

    if (m_window == nullptr)
    {
        const auto str = util::format_string("Unable to create window: %s", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", str.buffer.data(), nullptr);
        SDL_Quit();
        return;
    }

    const SDL_DisplayID id = SDL_GetDisplayForWindow(m_window);
    m_current_monitor = *SDL_GetCurrentDisplayMode(id);

    SDL_DestroyProperties(properties_id);
}
