#include "VulkanWindow.h"

#include <imgui_impl_sdl.h>

VulkanWindow::VulkanWindow()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	window = SDL_CreateWindow("SDL Vulkan Sample", 0, 0, 1280, 1024, SDL_WINDOW_VULKAN);

	ImGui_ImplSDL2_InitForVulkan(window);
	SDL_GetWindowSize(window, &width, &height);
}

VulkanWindow::~VulkanWindow()
{
	SDL_DestroyWindow(window);
	SDL_Quit();
}