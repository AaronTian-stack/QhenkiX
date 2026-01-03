#include "qhenki/input/input_manager.h"

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>

using namespace qhenki::input;

void InputManager::update(SDL_Window* window)
{
    memcpy(m_previous_keys.data(), m_current_keys.data(), sizeof(m_current_keys));

    m_mouse_flags_prev = m_mouse_flags;
    const auto state = SDL_GetKeyboardState(nullptr);
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
    {
        m_current_keys[i] = state[i];
    }

    m_mouse_position_prev = m_mouse_position;
    m_mouse_flags = SDL_GetMouseState(&m_mouse_position.x, &m_mouse_position.y); // Cached state

    // Always call SDL_GetRelativeMouseState to prevent accumulation
    // It accumulates since last call, not since relative mode was enabled
    float rel_x, rel_y;
    SDL_GetRelativeMouseState(&rel_x, &rel_y);

    const bool is_relative_mode = SDL_GetWindowRelativeMouseMode(window);
    if (is_relative_mode && m_was_relative_mode)
    {
        m_mouse_delta = {rel_x, rel_y};
    }
    else
    {
        // First frame of relative mode or not in relative mode
        m_mouse_delta = {0, 0};
    }
    m_was_relative_mode = is_relative_mode;
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
        m_mouse_scroll.x = dx;
        m_mouse_scroll.y = dy;
        break;
    }
    default:
        break;
    }
}

bool InputManager::is_key_down(const SDL_Scancode key) const
{
    return m_current_keys[key] != 0;
}

bool InputManager::is_key_just_pressed(const SDL_Scancode key) const
{
    return m_current_keys[key] != 0 && m_previous_keys[key] == 0;
}

auto InputManager::is_key_just_released(SDL_Scancode key) const -> bool
{
    return m_current_keys[key] == 0 && m_previous_keys[key] != 0;
}

auto InputManager::is_mouse_button_down(uint32_t button) const -> bool
{
    assert(button <= SDL_BUTTON_X2);
    return (m_mouse_flags & SDL_BUTTON_MASK(button)) != 0;
}

bool InputManager::is_mouse_button_just_pressed(const uint32_t button) const
{
    assert(button <= SDL_BUTTON_X2);
    return (m_mouse_flags & SDL_BUTTON_MASK(button)) != 0 && (m_mouse_flags_prev & SDL_BUTTON_MASK(button)) == 0;
}

bool InputManager::is_mouse_button_just_released(const uint32_t button) const
{
    assert(button <= SDL_BUTTON_X2);
    return (m_mouse_flags & SDL_BUTTON_MASK(button)) == 0 && (m_mouse_flags_prev & SDL_BUTTON_MASK(button)) != 0;
}

const XMFLOAT2& InputManager::get_mouse_delta() const
{
    return m_mouse_delta;
}

const XMFLOAT2& InputManager::get_mouse_scroll() const
{
    return m_mouse_scroll;
}
