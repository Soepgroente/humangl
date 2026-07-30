#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../type_aliases.hpp"

#include <cstdint>


namespace ve {

class VulkanWindow
{
	public:

	VulkanWindow() = delete;
	VulkanWindow(const char* title, bool fullScreen = false, i32 width = 800, i32 height = 600);
	VulkanWindow(const VulkanWindow& other) = delete;
	VulkanWindow(VulkanWindow&& other);
	VulkanWindow& operator=(const VulkanWindow& other) = delete;
	VulkanWindow& operator=(VulkanWindow&& other) = delete;
	~VulkanWindow();

	GLFWwindow*	getGLFWwindow() const noexcept { return window; }
	VkExtent2D	getWindowSize( void ) const noexcept;

	bool	shouldClose() const noexcept { return glfwWindowShouldClose(window); }
	void	createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) const;
	void	resetWindowSize(i32 width, i32 height) noexcept;
	void	toggleFullscreen() noexcept;
	bool	isFullscreenWindow() const noexcept { return glfwGetWindowMonitor(window) != nullptr; }

	private:

	i32	widthNotFullscreen;
	i32	heightNotFullscreen;
	i32	xPosNotFullscreen;
	i32	yPosNotFullscreen;

	GLFWmonitor*		monitor{nullptr};
	const GLFWvidmode*	monitorInfo;
	GLFWwindow*			window{nullptr};
};

}