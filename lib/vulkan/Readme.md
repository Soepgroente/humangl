This is a library that aims to make interacting with Vulkan a little bit easier. To create something, take the following steps (in order):

1)	An instance of the ve::VulkanWindow class will handle the window management using GLFW of the application. Parameters sent are:
	*	const char* title
	*	bool fullScreen
	*	width/height (default 800x600)

2)	An instance of the ve::VulkanDevice class will take the VulkanWindow as input. In the constructor of the class it calls functions that do the following:
	*	initialize vulkan with vkCreateInstance()
	*	setup the debug layer (enable/disable by compiling with or without -DNDebug flag in Makefile)
	*	
	*	looks through all physical devices and determines which is most suitable for rendering (ideally a dedicated GPU)
	*	creates a logical device to interface with the picked physical device
