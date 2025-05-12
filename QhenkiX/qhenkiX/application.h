#pragma once

#include "graphics/displaywindow.h"
#include <smartpointer.h>
#include "graphics/qhenki/context.h"
#include "graphics/qhenki/descriptor_heap.h"
#include "graphics/qhenki/descriptor_table.h"
#include <thread>

namespace qhenki::gfx
{
	enum class API
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
public:
	static constexpr UINT m_frames_in_flight = 2;

private:
	// Audio
	// Input
	// Files 
	// Preferences
	std::thread::id m_main_thread_id{};
	qhenki::gfx::API m_graphics_api_ = qhenki::gfx::API::D3D11;
protected:
	UINT m_frame_index_ = 0;
	DisplayWindow m_window_;
	uPtr<qhenki::gfx::Context> m_context_ = nullptr;
	qhenki::gfx::Swapchain m_swapchain_{};
	qhenki::gfx::Queue m_graphics_queue_{}; // A graphics queue is given to the application by default
	qhenki::gfx::DescriptorHeap m_rtv_heap{}; // Default RTV heap that also contains swapchain descriptors

	qhenki::gfx::Fence m_fence_frame_ready_{};
	std::array<uint64_t, m_frames_in_flight> m_fence_frame_ready_val_{0, 0};

	virtual void init_display_window();

	virtual void create() {}
	virtual void render() {}
	virtual void resize(int width, int height) {}
	virtual void destroy() {}

public:
	UINT get_frame_index() const { return m_frame_index_; }
	void increment_frame_index() { m_frame_index_ = (m_frame_index_ + 1) % m_frames_in_flight; }

	bool is_main_thread() const { return std::this_thread::get_id() == m_main_thread_id; }

	// Call this from the main thread
	void run(qhenki::gfx::API api);
	[[nodiscard]] qhenki::gfx::API get_graphics_api() const { return m_graphics_api_; }
	virtual ~Application() = 0;
};

