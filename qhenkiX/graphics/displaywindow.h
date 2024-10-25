#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>
#include <string>
#include <vector>
#include <DirectXMath.h>

#define NOMINMAX
#include <windows.h>

using namespace DirectX;

struct DisplayInfo
{
	int width;
	int height;
	int refresh_rate;
	bool fullscreen;
	bool undecorated;
	bool resizable;
	std::string title;
};

/**
 * Manages display mode, monitors, window.
 */
class DisplayWindow
{
	SDL_Window* window_ = nullptr;
	SDL_DisplayMode current_monitor_ = {};
	DisplayInfo display_info_ = {};

	SDL_SysWMinfo wmInfo = {};
	HWND hwnd_ = nullptr;

	void create_window_internal(const DisplayInfo& info, int monitor_index);

public:
	void create_window(const DisplayInfo& info, int monitor_index);

	SDL_DisplayMode get_current_monitor() const;
	static std::vector<SDL_DisplayMode> get_monitors();

	[[nodiscard]] XMUINT2 get_display_size() const;

	const DisplayInfo& get_display_info() const;
	HWND get_window_handle();

	bool set_fullscreen(bool fullscreen);
	bool set_resolution(int width, int height);

	void set_title(const char* title);

	void set_decoration(bool undecorated);

	void set_resizable(bool resizable);

	void wait();

	void destroy();

	friend class Application;
};

