#include "Humanimation.hpp"

Humanimation::Humanimation() :
	window{"Humangl", false, 800, 600},
	device{window, "humangl"},
	renderer{window, device}
{

}

Humanimation::~Humanimation()
{
	glfwTerminate();
}

void	Humanimation::run()
{
	while (window.shouldClose() == false)
	{
		glfwPollEvents();
	}
}