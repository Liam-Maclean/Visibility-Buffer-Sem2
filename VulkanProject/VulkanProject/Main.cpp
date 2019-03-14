#pragma once


#define TINYOBJLOADER_IMPLEMENTATION
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
