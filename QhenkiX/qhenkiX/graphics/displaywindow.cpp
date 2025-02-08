#include <stdexcept>

#include "displaywindow.h"
#include <SDL_syswm.h>

void DisplayWindow::create_window(const DisplayInfo& info, int monitor_index)
{
	create_window_internal(info, monitor_index);
}

SDL_DisplayMode DisplayWindow::get_current_monitor() const
{
	return current_monitor_;
}

std::vector<SDL_DisplayMode> DisplayWindow::get_monitors()
{
	std::vector<SDL_DisplayMode> modes;
	const int num_displays = SDL_GetNumVideoDisplays();
	for (int i = 0; i < num_displays; i++)
	{
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(i, &mode);
		modes.push_back(mode);
	}
	return modes;
}

XMUINT2 DisplayWindow::get_display_size() const
{
	return {static_cast<uint32_t>(display_info_.width), static_cast<uint32_t>(display_info_.height)};

}

const DisplayInfo& DisplayWindow::get_display_info() const
{
	return display_info_;
}

HWND DisplayWindow::get_window_handle()
{
	return hwnd_;
}

bool DisplayWindow::set_fullscreen(bool fullscreen)
{
	if (SDL_SetWindowFullscreen(window_, fullscreen ? SDL_WINDOW_FULLSCREEN : 0) != 0)
	{
		SDL_Log("Unable to set fullscreen: %s", SDL_GetError());
		return false;
	}
	display_info_.fullscreen = fullscreen;
	return true;
}

bool DisplayWindow::set_resolution(int width, int height)
{
	SDL_SetWindowSize(window_, width, height);
	display_info_.width = width;
	display_info_.height = height;
	return true;
}

void DisplayWindow::set_title(const char* title)
{
	SDL_SetWindowTitle(window_, title);
	display_info_.title = title;
}

void DisplayWindow::set_decoration(bool undecorated)
{
	SDL_SetWindowBordered(window_, undecorated ? SDL_FALSE : SDL_TRUE);
	display_info_.undecorated = undecorated;
}

void DisplayWindow::set_resizable(bool resizable)
{
	SDL_SetWindowResizable(window_, resizable ? SDL_TRUE : SDL_FALSE);
	display_info_.resizable = resizable;
}

void DisplayWindow::wait()
{
	//SDL_Delay(static_cast<int>(1.f / display_info.refresh_rate));
}

DisplayWindow::~DisplayWindow()
{
	SDL_DestroyWindow(window_);
	SDL_Quit();
}

void DisplayWindow::create_window_internal(const DisplayInfo& info, int monitor_index)
{
	// For now just choose primary monitor. The developer can create a menu to let a user choose a monitor, or the program can try picking one on its own.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		throw std::runtime_error("Unable to initialize SDL");
	}

	window_ = SDL_CreateWindow(info.title.c_str(),
		SDL_WINDOWPOS_CENTERED_DISPLAY(monitor_index), // Centered on primary display
		SDL_WINDOWPOS_CENTERED_DISPLAY(monitor_index),
		info.width,
		info.height,
		(info.fullscreen ? SDL_WINDOW_FULLSCREEN : 0) |
		(info.undecorated ? SDL_WINDOW_BORDERLESS : 0) |
		(info.resizable ? SDL_WINDOW_RESIZABLE : 0) | 
		SDL_WINDOW_ALLOW_HIGHDPI);

	display_info_ = info;

	if (window_ == nullptr)
	{
		SDL_Log("Unable to create window: %s", SDL_GetError());
		SDL_Quit();
		throw std::runtime_error("Unable to create window");
	}

	SDL_GetCurrentDisplayMode(monitor_index, &current_monitor_);

	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window_, &wmInfo);
	hwnd_ = (HWND)wmInfo.info.win.window;
}
