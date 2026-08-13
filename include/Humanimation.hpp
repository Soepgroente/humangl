#pragma once

#include "Scene.hpp"
#include "VulkanEngine.hpp"

class Humanimation
{
	public:

	Humanimation();
	~Humanimation();
	Humanimation(const Humanimation&) = delete;
	Humanimation(const Humanimation&&) = delete;
	void	operator=(const Humanimation&) = delete;

	void	run();

	private:

	Scene scene;

	ve::VulkanWindow	window;
	ve::VulkanDevice	device;
	ve::VulkanRenderer	renderer;
};