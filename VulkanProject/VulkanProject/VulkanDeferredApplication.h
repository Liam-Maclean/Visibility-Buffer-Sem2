#pragma once
#include "Window.h"
#include "Renderer.h"
#include "glm/glm.hpp"
#include "Camera.h"
#include "Plane.h"
#include "PlaneMesh.h"
#include "ImportedModel.h"
#include "ImGUIInterface.h"
#include <chrono>
#include <iostream>
#define TEX_DIMENSIONS 2048;

class VulkanDeferredApplication : public VulkanWindow
{
public:
	VulkanDeferredApplication(Renderer* renderer, int width, int height)
		:VulkanWindow(renderer, width, height)
	{
		InitialiseVulkanApplication();
	}
	~VulkanDeferredApplication();

	struct uboVS {
		glm::mat4 projection;
		glm::mat4 view;
	};
	struct vertice {
		VkPipelineVertexInputStateCreateInfo inputState;
		VkVertexInputBindingDescription bindingDescriptions;
		std::array<VkVertexInputAttributeDescription,3> attributeDescriptions;
	};

	struct UboDataDynamic
	{
		glm::mat4 *model = nullptr;
	} uboDataDynamic;

	struct UboTextureDataDynamic
	{
		VkSampler *sampler = nullptr;
	} uboTextureDataDynamic;


	struct {
		VkDescriptorSet cube;
		VkDescriptorSet house;
	}descriptorSets;




	void CreateCamera();
	void GiveImGuiStaticInformation();
	void InitialiseVulkanApplication();
	void Update() override;
	void DrawFrame() override;
	void prepareImGui();
	void _CreateGraphicsPipeline() override;
	void _CreateShadowPipeline();
	void UpdateUniformBuffer(uint32_t currentImage);
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	void _CreateGeometry();
	void _CreateRenderPass() override;
	void _CreateCommandBuffers() override;
	void _CreateVertexDescriptions();
	void _CreateDescriptorPool() override;
	void _CreateDescriptorSets() override;
	void _CreateDescriptorSetLayout() override;
	void _CreateShaderBuffer(VkDevice device, VkDeviceSize size, VkBuffer* vertexBuffer, VkDeviceMemory* vertexBufferMemory, VkBufferUsageFlagBits bufferStage, void *data);
	void SetUpUniformBuffers();
	void CreateShadowRenderPass();
	void CreateGBuffer();
	void CreateShadowPassCommandBuffers();
	void CreateDeferredCommandBuffers();


	ImGUIInterface *imGui = nullptr;

	vertice vertices;
	uboVS offScreenUniformVSData;
	uboVS fullScreenUniformVSData;

	ScreenTarget* screenTarget;
	PlaneMesh* planeMesh;
	ImportedModel* houseModel;


	size_t dynamicAlignment;
	size_t dynamicTextureAlignment;

	VkDescriptorSetLayout offScreenDescriptorSetLayout;
	VkDescriptorSetLayout deferredDescriptorSetLayout;

	VkDescriptorSetLayout shadowDescriptorSetLayout;
	VkDescriptorSet shadowDescriptorSet;
	vk::wrappers::ShadowFrameBuffer shadowFrameBuffer;

	VkDescriptorSet descriptorSet;
	VkDescriptorSet descriptorPool;
	
	vk::wrappers::GFrameBuffer deferredOffScreenFrameBuffer;
	VkSampler colorSampler;
	VkSampler shadowSampler;


	VkSemaphore presentCompleteSemaphore;
	VkSemaphore renderCompleteSemaphore;
	VkSemaphore offScreenSemaphore;
	VkSemaphore shadowSemaphore;

	VkCommandBuffer shadowCmdBuffer;
	VkCommandBuffer offScreenCmdBuffer;

	Camera* camera;


	glm::mat4 lightView;
	glm::mat4 lightProjection;
	glm::mat4 finalLightMatrix;
	glm::mat4 inverseFinalLightMatrix;

	vk::wrappers::Buffer cameraEyeBuffer;
	glm::vec4 cameraEye;
	vk::wrappers::Buffer lightViewMatrixBuffer;
	vk::wrappers::Buffer lightViewMatrixBuffershading;
	glm::mat4 lightViewMatrixShadowPass;


	vk::wrappers::Buffer dynamicUboBuffer;
	vk::wrappers::Buffer dynamicTextureUboBuffer;

	vk::wrappers::Buffer fullScreenVertexUBOBuffer;
	vk::wrappers::Buffer offScreenVertexUBOBuffer;

	VkQueryPool queryPool;

	bool LeftMouseDown = true;
	bool cameraUpdate = false;

	double frameTimeMRT;
	double frameTimeShading;

	int glfwOldKey;
	int glfwNewKey;

};

