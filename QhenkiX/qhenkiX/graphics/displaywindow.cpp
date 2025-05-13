#include <stdexcept>

#include "displaywindow.h"

using namespace qhenki;

void DisplayWindow::create_window(const DisplayInfo& info, int monitor_index)
{
	create_window_internal(info, monitor_index);
}

SDL_DisplayMode DisplayWindow::get_current_monitor() const
{
	return m_current_monitor_;
}

std::vector<SDL_DisplayMode> DisplayWindow::get_monitors()
{
	std::vector<SDL_DisplayMode> modes;
	int display_count;
	const auto displays = SDL_GetDisplays(&display_count);
	for (int i = 0; i < display_count; i++)
	{
		modes.push_back(*SDL_GetCurrentDisplayMode(displays[i]));
	}
	return modes;
}

XMUINT2 DisplayWindow::get_display_size() const
{
	return {static_cast<uint32_t>(m_display_info_.width), static_cast<uint32_t>(m_display_info_.height)};

}

const DisplayInfo& DisplayWindow::get_display_info() const
{
	return m_display_info_;
}

HWND DisplayWindow::get_window_handle()
{
	return m_hwnd_;
}

bool DisplayWindow::set_fullscreen(bool fullscreen)
{
	if (SDL_SetWindowFullscreen(m_window_, fullscreen) != 0)
	{
		SDL_Log("Unable to set fullscreen: %s", SDL_GetError());
		return false;
	}
	m_display_info_.fullscreen = fullscreen;
	return true;
}

bool DisplayWindow::set_resolution(const int width, const int height)
{
	SDL_SetWindowSize(m_window_, width, height);
	m_display_info_.width = width;
	m_display_info_.height = height;
	return true;
}

void DisplayWindow::set_title(const char* title)
{
	SDL_SetWindowTitle(m_window_, title);
	m_display_info_.title = title;
}

void DisplayWindow::set_decoration(const bool undecorated)
{
	SDL_SetWindowBordered(m_window_, undecorated);
	m_display_info_.undecorated = undecorated;
}

void DisplayWindow::set_resizable(bool resizable)
{
	SDL_SetWindowResizable(m_window_, resizable);
	m_display_info_.resizable = resizable;
}

void DisplayWindow::wait()
{
	//SDL_Delay(static_cast<int>(1.f / display_info.refresh_rate));
}

DisplayWindow::~DisplayWindow()
{
	SDL_DestroyWindow(m_window_);
	SDL_Quit();
}

void DisplayWindow::create_window_internal(const DisplayInfo& info, int monitor_index)
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		throw std::runtime_error("Unable to initialize SDL");
	}

	// TODO: depending on backend need SDL_WINDOW_VULKAN, SDL_WINDOW_METAL, etc
	// TODO: choose monitor to create window on

	SDL_PropertiesID properties_id = SDL_CreateProperties();
	SDL_SetBooleanProperty(properties_id, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, info.fullscreen);

	SDL_SetNumberProperty(properties_id, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, info.width);
	SDL_SetNumberProperty(properties_id, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, info.height);

	SDL_SetNumberProperty(properties_id, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(properties_id, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);

	SDL_SetBooleanProperty(properties_id, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, info.undecorated);
	SDL_SetBooleanProperty(properties_id, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, info.resizable);

	SDL_SetStringProperty(properties_id, SDL_PROP_WINDOW_CREATE_TITLE_STRING, info.title.c_str());

	m_window_ = SDL_CreateWindowWithProperties(properties_id);

	m_display_info_ = info;

	if (m_window_ == nullptr)
	{
		SDL_Log("Unable to create window: %s", SDL_GetError());
		SDL_Quit();
		throw std::runtime_error("Unable to create window");
	}

	const SDL_DisplayID id = SDL_GetDisplayForWindow(m_window_);
	m_current_monitor_ = *SDL_GetCurrentDisplayMode(id);

	const auto window_properties = SDL_GetWindowProperties(m_window_);

	m_hwnd_ = static_cast<HWND>(SDL_GetPointerProperty(window_properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
	if (m_hwnd_ == nullptr)
	{
		SDL_Log("Unable to get window handle: %s", SDL_GetError());
		SDL_Quit();
		throw std::runtime_error("Unable to get window handle");
	}
	
	SDL_DestroyProperties(properties_id);
}
