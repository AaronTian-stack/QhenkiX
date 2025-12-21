#pragma once
#include <array>
#include <DirectXMath.h>
#include <SDL3/SDL.h>

using namespace DirectX;

namespace qhenki
{
	class InputManager
	{
		std::array<bool, SDL_SCANCODE_COUNT> m_current_keys{};
		std::array<bool, SDL_SCANCODE_COUNT> m_previous_keys{};
		XMFLOAT2 m_mouse_position{ 0, 0 };
		XMFLOAT2 m_mouse_position_prev{ 0, 0 };
		XMFLOAT2 m_mouse_delta{ 0, 0 };
		XMFLOAT2 m_mouse_scroll{ 0, 0 };
		uint32_t m_mouse_flags{ 0 };
		uint32_t m_mouse_flags_prev{ 0 };
	public:
		void update(SDL_Window* window);
		void handle_extra_events(const SDL_Event& event);

		bool is_key_down(SDL_Scancode key) const;
		bool is_key_just_pressed(SDL_Scancode key) const;
		bool is_key_just_released(SDL_Scancode key) const;

		bool is_mouse_button_down(uint32_t button) const;
		bool is_mouse_button_just_pressed(uint32_t button) const;
		bool is_mouse_button_just_released(uint32_t button) const;

		void reset_mouse_scroll() { m_mouse_scroll = { 0, 0 }; }

		const XMFLOAT2& get_mouse_delta() const;
		const XMFLOAT2& get_mouse_scroll() const;
	};
}
