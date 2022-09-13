#pragma once

#include "SDL2/SDL.h"

struct VulkanWindow
{
	SDL_Window* window;
	int width;
	int height;

	VulkanWindow();

	~VulkanWindow();
};