#include "MouseInput.hpp"

void	MouseInput::reset() noexcept
{
	mouseStates.fill(false);
	buttonsPressed.fill(false);
	buttonsReleased.fill(false);
}
