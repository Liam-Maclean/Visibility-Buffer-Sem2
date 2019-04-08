#pragma once
#include "imgui.h"
#include <Renderer.h>
#include <Shared.h>


class ImGUIInterface
{
public:

	struct UISettings {

		std::string windowTitle = "";
		std::string modelName = "Model: ";
		bool displayModels = true;
		bool cameraActive = false;

		bool normalMode = false, positionMode = false, albedoMode = false, VBIDMode = false;

		int vertices = 0, indices = 0, face = 0;
		
		float nanosecondsMRTPass = 0;
		float nanosecondsShadingPass = 0;

		double millisecondsMRT, microsecondsMRT, nanosecondsMRT;
		double millisecondsShading, microsecondsShading, nanosecondsShading;

		std::array<float, 50> frameTimesMRT{};
		double frameTimeMinMRT = 0.0f, frameTimeMaxMRT = 0.0f;

		std::array<float, 50> frameTimesShading{};
		double frameTimeMinShading = 0.0f, frameTimeMaxShading = 0.0f;
		float lightTimer;
	}uiSettings;

	ImGUIInterface(Renderer* renderer);
	~ImGUIInterface();

	void init(float width, float height, std::string title, ImGuiMode mode);
	void initResources(VkRenderPass renderPass, VkQueue copyQueue);
	void newFrame(double frameTimerMRT, double frameTimerShading, bool updateFrameGraph);
	void UpdateImGuiInformation(bool cameraActive);
	void updateBuffers();
	void DrawFrame(VkCommandBuffer commandBuffer);
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	
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

	ImGuiMode imGuiMode;


};

