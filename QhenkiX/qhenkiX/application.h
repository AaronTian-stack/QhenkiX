#pragma once

#include "graphics/displaywindow.h"
#include "graphics/d3d11/d3d11_context.h"

namespace qhenki
{
	enum GraphicsAPI
	{
		D3D11,
		D3D12,
		Vulkan,
	};
}

/**
 * Encapsulates the objects that control DisplayWindow, Audio, Input, File System, Preferences.
 * Sets up default states for the objects, but initialization can be overridden.
 */
class Application
{
	//Audio
	//Input
	//Files 
	//Preferences

	qhenki::GraphicsAPI m_graphics_api_;
protected:
	DisplayWindow m_window_;
	uPtr<qhenki::Context> m_context_ = nullptr;
	qhenki::Swapchain m_swapchain_{};
	qhenki::Queue m_graphics_queue_{};

	virtual void init_display_window();

	virtual void create() {}
	virtual void render() {}
	virtual void resize(int width, int height) {}
	virtual void destroy() {}

public:
	void run(qhenki::GraphicsAPI api);
	[[nodiscard]] qhenki::GraphicsAPI get_graphics_api() const { return m_graphics_api_; }
};

