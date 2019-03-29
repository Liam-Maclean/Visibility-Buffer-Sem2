#pragma once
#include "vulkan/vulkan.h"
#include <string>
#include <vector>

//Tools for vulkan
namespace tools
{
	void ErrorCheck(VkResult result);
	std::vector<char> ReadShaderFile(const std::string& filename);
	VkShaderModule loadShader(const char * fileName, VkDevice device);
	void setImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
	void setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
	//void _CopyBuffer(Renderer* renderer, VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	//void _CreateBuffer(Renderer* renderer, VkCommandBuffer commandBuffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
	//void _CreateShaderBuffer(Renderer* renderer, VkCommandBuffer commandBuffer, VkDevice device, VkDeviceSize size, VkBuffer * buffer, VkDeviceMemory* memory, VkBufferUsageFlagBits bufferStage, void* data);
}

