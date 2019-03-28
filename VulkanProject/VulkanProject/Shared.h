#pragma once

#include <iostream>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <fstream>
#include "glm/glm.hpp"

class BaseModel;

namespace vk
{
	namespace wrappers
	{
		//Wrapper class for buffer holding
		struct Buffer
		{
			VkDevice device;
			VkBuffer buffer;
			VkDeviceMemory memory;
			VkDescriptorBufferInfo descriptor;
			void* mapped = nullptr;


			void SetUpDescriptorSet()
			{
				descriptor.offset = 0;
				descriptor.buffer = buffer;
				descriptor.range = VK_WHOLE_SIZE;
			}

			VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
			{
				return vkMapMemory(device, memory, offset, size, 0, &mapped);
			}

			void unmap()
			{
				if (mapped)
				{
					vkUnmapMemory(device, memory);
					mapped = nullptr;
				}
			}

			VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
			{
				VkMappedMemoryRange mappedRange = {};
				mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				mappedRange.memory = memory;
				mappedRange.offset = offset;
				mappedRange.size = size;
				return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
			}

			void destroy()
			{
				if (buffer)
				{
					vkDestroyBuffer(device, buffer, nullptr);
				}
				if (memory)
				{
					vkFreeMemory(device, memory, nullptr);
				}
			}

		};

		//FrameBuffer attachment Wrapper
		struct FrameBufferAttachment
		{
			VkImage image;
			VkDeviceMemory mem;
			VkImageView view;
			VkFormat format;
		};

		//G FrameBuffer Wrapper
		struct GFrameBuffer
		{
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment position, normal, albedo, depth;
			VkRenderPass renderPass;
		};

		//FrameBuffer wrapper for shadow pass
		struct ShadowFrameBuffer
		{
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment depth;
			VkRenderPass renderPass;
		};

		//Visibility FrameBuffer wrapper
		struct VFrameBuffer
		{
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment VID, depth;
			VkRenderPass renderPass;
		};

		

		//Texture wrapper class
		struct Texture2D
		{
			VkImage image;
			VkImageView imageView;
			VkImageLayout imageLayout;
			VkDeviceMemory deviceMemory;
			uint32_t width, height;
			VkDescriptorImageInfo descriptor;
			VkSampler sampler;


			void SetUpDescriptor()
			{
				descriptor.imageLayout = imageLayout;
				descriptor.imageView = imageView;
				descriptor.sampler = sampler;
			}

		};

		struct PointLight
		{
			glm::vec4 position;
			glm::vec4 diffuse;
			glm::vec4 specular;

			float constant;
			float linear;
			float quadratic;
		};

		struct DirectionalLight
		{
			glm::vec4 direction;
			glm::vec4 diffuse;
			glm::vec4 specular;
		};

		struct SpotLight
		{
			glm::vec4 position;
			glm::vec4 diffuse;
			glm::vec3 target;
			float cutOff;
		};

		//Model wrapper class
		struct ModelBuffers
		{
			vk::wrappers::Buffer vertexBuffer;
			vk::wrappers::Buffer indexBuffer;
			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;
		};

		struct Model
		{
			VkDescriptorSet descriptorSet;
			BaseModel* model;
			vk::wrappers::Texture2D texture;
		};

	}

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
}