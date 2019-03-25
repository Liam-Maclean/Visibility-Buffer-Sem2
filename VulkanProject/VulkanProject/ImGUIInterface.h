#pragma once
#include <imgui.h>
#include <Renderer.h>
#include <Shared.h>

struct UISettings {
	bool displayModels = true;
	std::array<float, 50> frameTimes{};
	float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
}uiSettings;

class ImGUIInterface
{
public:
	ImGUIInterface(Renderer* renderer);
	~ImGUIInterface();

	void init(float width, float height);
	void initResources(VkRenderPass renderPass, VkQueue copyQueue);
	void newFrame(bool updateFrameGraph);
	void updateBuffers();
	void DrawFrame(VkCommandBuffer commandBuffer);

	struct PushConstBlock
	{
		glm::vec2 scale;
		glm::vec2 translate;
	}pushConstBlock;


private:
	VkSampler sampler;
	vk::wrappers::Buffer vertexBuffer;
	vk::wrappers::Buffer indexBuffer;
	int32_t vertexCount = 0;
	int32_t indexCount = 0;
	VkDeviceMemory fontMemory = VK_NULL_HANDLE;
	VkImage fontImage = VK_NULL_HANDLE;
	VkImageView fontView = VK_NULL_HANDLE;
	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	Renderer *pRenderer;

};

