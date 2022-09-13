#include "VulkanWindow.h"

VulkanWindow::VulkanWindow()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	window = SDL_CreateWindow("SDL Vulkan Sample", 0, 0, 800, 600, SDL_WINDOW_VULKAN);
	SDL_GetWindowSize(window, &width, &height);
}

VulkanWindow::~VulkanWindow()
{
	SDL_DestroyWindow(window);
	SDL_Quit();
}