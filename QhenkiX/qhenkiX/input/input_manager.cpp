#include "input_manager.h"

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>

using namespace qhenki;

void InputManager::update(SDL_Window* window)
{
	std::ranges::copy(current_keys_, previous_keys_.begin());
	mouse_flags_prev_ = mouse_flags_;
	const auto state = SDL_GetKeyboardState(nullptr);
	for (int i = 0; i < SDL_SCANCODE_COUNT; ++i)
	{
		current_keys_[i] = state[i];
	}
	mouse_position_prev_ = mouse_position_;
	mouse_flags_ = SDL_GetMouseState(&mouse_position_.x, &mouse_position_.y); // cached state

	if (SDL_GetWindowRelativeMouseMode(window))
	{
		mouse_flags_ = SDL_GetRelativeMouseState(&mouse_delta_.x, &mouse_delta_.y);
	}
	else
	{
		XMStoreFloat2(&mouse_delta_, XMLoadFloat2(&mouse_position_) - XMLoadFloat2(&mouse_position_prev_));
	}
}

void InputManager::handle_extra_events(const SDL_Event& event)
{
	switch (event.type)
	{
		case SDL_EVENT_MOUSE_WHEEL:
		{
			float dx = event.wheel.x;
			float dy = event.wheel.y;
			if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) 
			{
				dx = -dx;
				dy = -dy;
			}
			mouse_scroll_.x = dx;
			mouse_scroll_.y = dy;
			break;
		}
		default:
			break;
	}
}

bool InputManager::is_key_down(SDL_Scancode key) const
{
	return current_keys_[key] != 0;
}

bool InputManager::is_key_just_pressed(SDL_Scancode key) const
{
	return current_keys_[key] != 0 && previous_keys_[key] == 0;
}

bool InputManager::is_key_just_released(SDL_Scancode key) const
{
	return current_keys_[key] == 0 && previous_keys_[key] != 0;
}

bool InputManager::is_mouse_button_down(uint32_t button) const
{
	assert(button <= SDL_BUTTON_X2);
	return (mouse_flags_ & SDL_BUTTON_MASK(button)) != 0;
}

bool InputManager::is_mouse_button_just_pressed(uint32_t button) const
{
	assert(button <= SDL_BUTTON_X2);
	return (mouse_flags_ & SDL_BUTTON_MASK(button)) != 0 &&
		(mouse_flags_prev_ & SDL_BUTTON_MASK(button)) == 0;
}

bool InputManager::is_mouse_button_just_released(uint32_t button) const
{
	assert(button <= SDL_BUTTON_X2);
	return (mouse_flags_ & SDL_BUTTON_MASK(button)) == 0 &&
		(mouse_flags_prev_ & SDL_BUTTON_MASK(button)) != 0;
}

const XMFLOAT2& InputManager::get_mouse_delta() const
{
	return mouse_delta_;
}

const XMFLOAT2& InputManager::get_mouse_scroll() const
{
	return mouse_scroll_;
}
