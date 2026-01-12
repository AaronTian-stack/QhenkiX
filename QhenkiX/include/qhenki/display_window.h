#pragma once

#define SDL_MAIN_HANDLED
#include <DirectXMath.h>
#include <SDL3/SDL.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif
#include <vector>

using namespace DirectX;

namespace qhenki
{
struct DisplayInfo
{
    unsigned int width;
    unsigned int height;
    int refresh_rate;
    bool fullscreen;
    bool undecorated;
    bool resizable;
    const char* title;
};

/**
 * Manages display mode, monitors, window.
 */
class DisplayWindow
{
    SDL_Window* m_window = nullptr;
    SDL_DisplayMode m_current_monitor = {};
    DisplayInfo m_display_info = {};

    void create_window_internal(const DisplayInfo& info, int monitor_index);

public:
    void create_window(const DisplayInfo& info, int monitor_index);

    SDL_DisplayMode get_current_monitor() const;
    static std::vector<SDL_DisplayMode> get_monitors();

    XMUINT2 get_display_size() const;

    const DisplayInfo& get_display_info() const;

    SDL_Window* get_window() const;

    bool set_fullscreen(bool fullscreen);
    bool set_resolution(int width, int height);

    void set_title(const char* title);

    void set_decoration(bool undecorated);

    void set_resizable(bool resizable);

    ~DisplayWindow();

#if defined(_WIN32) || defined(_WIN64)
    HWND get_hwnd() const
    {
        const auto window_properties = SDL_GetWindowProperties(m_window);
        const auto hwnd = static_cast<HWND>(
            SDL_GetPointerProperty(window_properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
        return hwnd;
    }
#endif

    friend class Application;
};
} // namespace qhenki
