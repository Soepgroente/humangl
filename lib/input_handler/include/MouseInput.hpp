#pragma once

#include "Vectors.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>

#include "../type_aliases.hpp"

class MouseInput
{
	public:

	MouseInput() noexcept : cursorPos(0.0f) { this->reset(); };
	~MouseInput() noexcept = default;
	MouseInput(const MouseInput& other) = delete;
	MouseInput(MouseInput&& other) = delete;
	MouseInput& operator=(const MouseInput& other) = delete;
	MouseInput& operator=(MouseInput&& other) = delete;

	void		reset() noexcept;
	void		setCursorPos(const vec2& newPos) noexcept { this->cursorPos = newPos; };
	const vec2&	getCursorPos() const noexcept { return this->cursorPos; };

	static constexpr i32 maxButtons = GLFW_MOUSE_BUTTON_LAST + 1;

	std::array<bool, maxButtons>	mouseStates;
	std::array<bool, maxButtons>	buttonsPressed;
	std::array<bool, maxButtons>	buttonsReleased;

	vec2 cursorPos;
};
