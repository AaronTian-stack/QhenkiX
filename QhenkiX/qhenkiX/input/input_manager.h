#pragma once
#include <array>
#include <DirectXMath.h>
#include <SDL3/SDL.h>

using namespace DirectX;

namespace qhenki
{
	class InputManager
	{
		std::array<bool, SDL_SCANCODE_COUNT> current_keys_{};
		std::array<bool, SDL_SCANCODE_COUNT> previous_keys_{};
		XMFLOAT2 mouse_position_{ 0, 0 };
		XMFLOAT2 mouse_position_prev_{ 0, 0 };
		XMFLOAT2 mouse_delta_{ 0, 0 };
		XMFLOAT2 mouse_scroll_{ 0, 0 };
		uint32_t mouse_flags_{ 0 };
		uint32_t mouse_flags_prev_{ 0 };
	public:
		void update(SDL_Window* window);
		void handle_extra_events(const SDL_Event& event);

		bool is_key_down(SDL_Scancode key) const;
		bool is_key_just_pressed(SDL_Scancode key) const;
		bool is_key_just_released(SDL_Scancode key) const;

		bool is_mouse_button_down(uint32_t button) const;
		bool is_mouse_button_just_pressed(uint32_t button) const;
		bool is_mouse_button_just_released(uint32_t button) const;

		void reset_mouse_scroll() { mouse_scroll_ = { 0, 0 }; }

		const XMFLOAT2& get_mouse_delta() const;
		const XMFLOAT2& get_mouse_scroll() const;
	};
}
