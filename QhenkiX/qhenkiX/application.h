#pragma once

#include "graphics/displaywindow.h"
#include <smartpointer.h>
#include "graphics/qhenki/context.h"
#include "graphics/qhenki/descriptor_heap.h"
#include <thread>

namespace qhenki::gfx
{
	enum API
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
	// Audio
	// Input
	// Files 
	// Preferences
	std::thread::id m_main_thread_id;
	qhenki::gfx::API m_graphics_api_;
protected:
	UINT m_frame_index_ = 0;
	DisplayWindow m_window_;
	uPtr<qhenki::gfx::Context> m_context_ = nullptr;
	qhenki::gfx::Swapchain m_swapchain_{};
	qhenki::gfx::Queue m_graphics_queue_{}; // A graphics queue is given to the application by default
	qhenki::gfx::DescriptorHeap rtv_heap{};
	qhenki::gfx::DescriptorTable swapchain_targets{};

	virtual void init_display_window();

	virtual void create() {}
	virtual void render() {}
	virtual void resize(int width, int height) {}
	virtual void destroy() {}

public:
	static inline constexpr UINT m_frames_in_flight = 2;
	UINT get_frame_index() const { return m_frame_index_; }

	bool is_main_thread() const { return std::this_thread::get_id() == m_main_thread_id; }

	// Call this from the main thread
	void run(qhenki::gfx::API api);
	[[nodiscard]] qhenki::gfx::API get_graphics_api() const { return m_graphics_api_; }
	virtual ~Application() = 0;
};

