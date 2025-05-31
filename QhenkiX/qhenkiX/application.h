#pragma once

#include "graphics/display_window.h"
#include <smartpointer.h>
#include "graphics/qhenki/context.h"
#include "graphics/qhenki/descriptor_heap.h"
#include <thread>

#include "input/input_manager.h"

namespace qhenki
{
	namespace gfx
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
		std::thread::id m_main_thread_id{};
		gfx::API m_graphics_api_ = gfx::API::D3D11;
	protected:
		// Audio
		InputManager m_input_manager_{}; // Input
		// Files 
		// Preferences

		bool m_QUIT_ = false; // Set to true when the application should quit
		UINT m_frame_index_ = 0;
		DisplayWindow m_window_;
		uPtr<gfx::Context> m_context_ = nullptr;
		gfx::Swapchain m_swapchain_{};
		gfx::Queue m_graphics_queue_{}; // A graphics queue is given to the application by default
		gfx::DescriptorHeap m_rtv_heap_{}; // Default RTV heap that also contains swapchain descriptors

		gfx::Fence m_fence_frame_ready_{};
		std::array<uint64_t, m_frames_in_flight> m_fence_frame_ready_val_{ 0, 0 };

		virtual void init_display_window();

		virtual void create() {}
		// The swapchain is automatically resized before this is called
		virtual void render() {}
		virtual void resize(int width, int height) {}
		virtual void destroy() {}

	public:
		UINT get_frame_index() const { return m_frame_index_; }
		void increment_frame_index() { m_frame_index_ = (m_frame_index_ + 1) % m_frames_in_flight; }

		bool is_main_thread() const { return std::this_thread::get_id() == m_main_thread_id; }

		// Call this from the main thread
		void run(gfx::API api);
		[[nodiscard]] gfx::API get_graphics_api() const { return m_graphics_api_; }
		virtual ~Application() = 0;
	};
}
