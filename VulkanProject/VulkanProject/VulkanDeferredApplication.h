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
#define TEX_DIMENSIONS 2048
#define OFFSCREEN_WIDTH 1280 
#define OFFSCREEN_HEIGHT 720

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

	struct materialInfo {
		uint32_t materialID;
		uint32_t padding1;
	};

	struct vertexArgs {
		uint32_t vertexBase;
		uint32_t padding;
	};


	void CreateIndirectInformation();
	void CreateCamera();
	void CreateQueryPools();
	void GiveImGuiStaticInformation();
	void InitialiseVulkanApplication();
	void Update() override;
	void DrawFrame() override;
	void CreateImGui();
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
	void _CreateOffScreenColorRenderPass();
	void _CreateOffScreenColorPipeline();
	void _CreateOffScreenColorCommandBuffers();
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


	VkSemaphore presentCompleteSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
	VkSemaphore offScreenSemaphore = VK_NULL_HANDLE;
	VkSemaphore shadowSemaphore = VK_NULL_HANDLE;
	VkSemaphore offScreenColorSemaphore = VK_NULL_HANDLE;


	VkCommandBuffer shadowCmdBuffer = VK_NULL_HANDLE;
	VkCommandBuffer offScreenCmdBuffer = VK_NULL_HANDLE;
	VkCommandBuffer offScreenColorCmdBuffers = VK_NULL_HANDLE;


	Camera* camera;


	//INDIRECT INFORMATION
	std::vector<VkDrawIndexedIndirectCommand> indirectCommands;
	std::vector<materialInfo> materialIDs;
	std::vector<vertexArgs> startVertex;
	uint32_t indirectDrawCount;

	vk::wrappers::Buffer indirectCommandsBuffer;
	vk::wrappers::Buffer materialIDBuffer;

	glm::mat4 lightView;
	glm::mat4 lightProjection;
	glm::mat4 finalLightMatrix;
	glm::mat4 inverseFinalLightMatrix;

	vk::wrappers::Buffer cameraEyeBuffer;
	glm::vec4 cameraEye;
	vk::wrappers::Buffer lightViewMatrixBuffer;
	vk::wrappers::Buffer lightViewMatrixBuffershading;
	glm::mat4 lightViewMatrixShadowPass;
	uint32_t drawModeValue;


	vk::wrappers::Buffer dynamicUboBuffer;
	vk::wrappers::Buffer dynamicTextureUboBuffer;

	vk::wrappers::Buffer fullScreenVertexUBOBuffer;
	vk::wrappers::Buffer offScreenVertexUBOBuffer;

	VkQueryPool DeferredMRTQueryPool;
	VkQueryPool DeferredShadeQueryPool;






	bool LeftMouseDown = true;
	bool cameraUpdate = false;

	double frameTimeMRT;
	double frameTimeShading;

	int glfwOldKey;
	int glfwNewKey;

	

	//Off screen render pass stuff
	vk::wrappers::OSColorFrameBuffer offScreenColorFrameBuffer;


	uint32_t queryTimeMRTStart;
	uint32_t queryTimeMRTEnd;

	uint32_t queryTimeShadeStart;
	uint32_t queryTimeShadeEnd;

};

