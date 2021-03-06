#pragma once


#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Renderer.h"
#include "Window.h"
#include "VulkanDeferredApplication.h"
#include "VisibilityBuffer.h"

int main()
{
	Renderer r;
	VulkanDeferredApplication va(&r,1280,720);
	return 0;
}
