#include <stdexcept>

#include "displaywindow.h"
#include <SDL_syswm.h>

void DisplayWindow::create_window(const DisplayInfo& info, int monitor_index)
{
	create_window_internal(info, monitor_index);
}

SDL_DisplayMode DisplayWindow::get_current_monitor() const
{
	return current_monitor;
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

const DisplayInfo& DisplayWindow::get_display_info()
{
	return display_info;
}

HWND DisplayWindow::get_window_handle()
{
	return hwnd;
}

bool DisplayWindow::set_fullscreen(bool fullscreen)
{
	if (SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0) != 0)
	{
		SDL_Log("Unable to set fullscreen: %s", SDL_GetError());
		return false;
	}
	display_info.fullscreen = fullscreen;
	return true;
}

bool DisplayWindow::set_resolution(int width, int height)
{
	SDL_SetWindowSize(window, width, height);
	display_info.width = width;
	display_info.height = height;
	return true;
}

void DisplayWindow::set_title(const char* title)
{
	SDL_SetWindowTitle(window, title);
	display_info.title = title;
}

void DisplayWindow::set_decoration(bool undecorated)
{
	SDL_SetWindowBordered(window, undecorated ? SDL_FALSE : SDL_TRUE);
	display_info.undecorated = undecorated;
}

void DisplayWindow::set_resizable(bool resizable)
{
	SDL_SetWindowResizable(window, resizable ? SDL_TRUE : SDL_FALSE);
	display_info.resizable = resizable;
}

void DisplayWindow::wait()
{
	SDL_Delay(static_cast<int>(1.f / display_info.refresh_rate));
}

void DisplayWindow::destroy()
{
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void DisplayWindow::create_window_internal(const DisplayInfo& info, int monitor_index)
{
	display_info = info;

	// For now just choose primary monitor. The developer can create a menu to let a user choose a monitor, or the program can try picking one on its own.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		throw std::runtime_error("Unable to initialize SDL");
	}

	window = SDL_CreateWindow(info.title.c_str(),
		SDL_WINDOWPOS_CENTERED_DISPLAY(monitor_index), // Centered on primary display
		SDL_WINDOWPOS_CENTERED_DISPLAY(monitor_index),
		info.width,
		info.height,
		(info.fullscreen ? SDL_WINDOW_FULLSCREEN : 0) |
		(info.undecorated ? SDL_WINDOW_BORDERLESS : 0) |
		(info.resizable ? SDL_WINDOW_RESIZABLE : 0) | 
		SDL_WINDOW_ALLOW_HIGHDPI);

	if (window == nullptr)
	{
		SDL_Log("Unable to create window: %s", SDL_GetError());
		SDL_Quit();
		throw std::runtime_error("Unable to create window");
	}

	SDL_GetCurrentDisplayMode(monitor_index, &current_monitor);

	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	hwnd = (HWND)wmInfo.info.win.window;
}
