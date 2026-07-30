#include "KeyboardInput.hpp"

KeyboardInput::KeyboardInput() noexcept
{
	this->reset();
	keysPressed.fill(false);
}

void	KeyboardInput::reset() noexcept
{
	keyStates.fill(false);
	keysReleased.fill(false);
}
