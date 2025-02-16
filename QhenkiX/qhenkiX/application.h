#pragma once

#include "graphics/displaywindow.h"
#include "graphics/d3d11/d3d11_context.h"

namespace qhenki::graphics
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

	qhenki::graphics::GraphicsAPI m_graphics_api_;
protected:
	UINT m_frame_index_ = 0;
	DisplayWindow m_window_;
	uPtr<qhenki::graphics::Context> m_context_ = nullptr;
	qhenki::graphics::Swapchain m_swapchain_{};
	qhenki::graphics::Queue m_graphics_queue_{}; // A graphics queue is given to the application by default

	virtual void init_display_window();

	virtual void create() {}
	virtual void render() {}
	virtual void resize(int width, int height) {}
	virtual void destroy() {}

public:
	static inline constexpr UINT m_frames_in_flight = 2;
	UINT get_frame_index() const { return m_frame_index_; }

	void run(qhenki::graphics::GraphicsAPI api, std::thread::id main_thread);
	[[nodiscard]] qhenki::graphics::GraphicsAPI get_graphics_api() const { return m_graphics_api_; }
	virtual ~Application() = 0;
};

