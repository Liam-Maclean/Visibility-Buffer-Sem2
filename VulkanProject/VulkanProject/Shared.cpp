#include "Shared.h"

void vk::tools::ErrorCheck(VkResult result)
{
	if (result < 0)
	{
		switch (result)
		{
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			std::cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			std::cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
			break;
		case VK_ERROR_INITIALIZATION_FAILED:
			std::cout << "VK_ERROR_INITIALIZATION_FAILED" << std::endl;
			break;
		case VK_ERROR_DEVICE_LOST:
			std::cout << "VK_ERROR_DEVICE_LOST" << std::endl;
			break;
		case VK_ERROR_MEMORY_MAP_FAILED:
			std::cout << "VK_ERROR_MEMORY_MAP_FAILED" << std::endl;
			break;
		case VK_ERROR_LAYER_NOT_PRESENT:
			std::cout << "VK_ERROR_LAYER_NOT_PRESENT" << std::endl;
			break;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			std::cout << "VK_ERROR_EXTENSION_NOT_PRESENT" << std::endl;
			break;
		case VK_ERROR_FEATURE_NOT_PRESENT:
			std::cout << "VK_ERROR_FEATURE_NOT_PRESENT" << std::endl;
			break;
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			std::cout << "VK_ERROR_INCOMPATIBLE_DRIVER" << std::endl;
			break;
		case VK_ERROR_TOO_MANY_OBJECTS:
			std::cout << "VK_ERROR_TOO_MANY_OBJECTS" << std::endl;
			break;
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			std::cout << "VK_ERROR_FORMAT_NOT_SUPPORTED" << std::endl;
			break;
		case VK_ERROR_FRAGMENTED_POOL:
			std::cout << "VK_ERROR_FRAGMENTED_POOL" << std::endl;
			break;
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			std::cout << "VK_ERROR_OUT_OF_POOL_MEMORY" << std::endl;
			break;
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
			std::cout << "VK_ERROR_INVALID_EXTERNAL_HANDLE" << std::endl;
			break;
		case VK_ERROR_SURFACE_LOST_KHR:
			std::cout << "VK_ERROR_SURFACE_LOST_KHR" << std::endl;
			break;
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			std::cout << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << std::endl;
			break;
		case VK_SUBOPTIMAL_KHR:
			std::cout << "VK_SUBOPTIMAL_KHR" << std::endl;
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			std::cout << "VK_ERROR_OUT_OF_DATE_KHR" << std::endl;
			break;
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			std::cout << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" << std::endl;
			break;
		case VK_ERROR_VALIDATION_FAILED_EXT:
			std::cout << "VK_ERROR_VALIDATION_FAILED_EXT" << std::endl;
			break;
		default:
			break;
		}
		assert(0 && "Vulkan runtime error.");
	}
}

//reads a shader file 
std::vector<char> vk::tools::ReadShaderFile(const std::string & filename)
{
	//loads binary data from the file(Binary), starts at the end of the file (Ate)
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	//if the file doesn't open
	if (!file.is_open())
	{
		//Runtime error
		throw std::runtime_error("Failed to open file " + filename);
	}

	//reads the position of file (since we're at the end we can tell the size of the file)
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	//returns back to the beginning of the file and reads all the bytes at once
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	//close and return the bytes from the file
	file.close();
	return buffer;
}

VkShaderModule vk::tools::loadShader(const char *fileName, VkDevice device)
{
	std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

	if (is.is_open())
	{
		size_t size = is.tellg();
		is.seekg(0, std::ios::beg);
		char* shaderCode = new char[size];
		is.read(shaderCode, size);
		is.close();

		assert(size > 0);

		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;

		vk::tools::ErrorCheck(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

		delete[] shaderCode;

		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << std::endl;
		return VK_NULL_HANDLE;
	}
}

//void vk::tools::_CopyBuffer(Renderer * renderer, VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
//{
//	VkCommandBuffer commandBuffer = commandBuffer;
//
//	VkBufferCopy copyRegion = {};
//	copyRegion.size = size;
//	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
//
//	_EndSingleTimeCommands(commandBuffer);
//}
//
//void vk::tools::_CreateBuffer(Renderer * renderer, VkCommandBuffer commandBuffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
//{
//	
//	VkBufferCreateInfo buffer_create_info = {};
//	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//	buffer_create_info.size = size;
//	buffer_create_info.usage = usage;
//	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//	vk::tools::ErrorCheck(vkCreateBuffer(renderer->GetVulkanDevice(), &buffer_create_info, nullptr, &buffer));
//
//	VkMemoryRequirements mem_requirements;
//	vkGetBufferMemoryRequirements(renderer->GetVulkanDevice(), buffer, &mem_requirements);
//
//	VkMemoryAllocateInfo memory_allocate_info = {};
//	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	memory_allocate_info.allocationSize = mem_requirements.size;
//	memory_allocate_info.memoryTypeIndex = renderer->_GetMemoryType(mem_requirements.memoryTypeBits, properties);
//
//	vk::tools::ErrorCheck(vkAllocateMemory(renderer->GetVulkanDevice(), &memory_allocate_info, nullptr, &bufferMemory));
//
//	vkBindBufferMemory(renderer->GetVulkanDevice(), buffer, bufferMemory, 0);
//}
//
//void vk::tools::_CreateShaderBuffer(Renderer * renderer, VkCommandBuffer commandBuffer, VkDevice device, VkDeviceSize size, VkBuffer * buffer, VkDeviceMemory * memory, VkBufferUsageFlagBits bufferStage, void * data)
//{
//	VkDeviceSize bufferSize = size;
//
//	VkBuffer stagingBuffer;
//	VkDeviceMemory stagingBufferMemory;
//	vk::tools::_CreateBuffer(renderer, commandBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
//
//	void* tempData;
//	vkMapMemory(renderer->GetVulkanDevice(), stagingBufferMemory, 0, bufferSize, 0, &tempData);
//	memcpy(tempData, data, (size_t)bufferSize);
//	vkUnmapMemory(renderer->GetVulkanDevice(), stagingBufferMemory);
//
//	vk::tools::_CreateBuffer(renderer, commandBuffer, bufferSize, bufferStage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, *buffer, *memory);
//	_CopyBuffer(renderer, commandBuffer, stagingBuffer, *buffer, bufferSize);
//
//	vkDestroyBuffer(renderer->GetVulkanDevice(), stagingBuffer, nullptr);
//	vkFreeMemory(renderer->GetVulkanDevice(), stagingBufferMemory, nullptr);
//}

