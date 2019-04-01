#pragma once
#include "Window.h"
#include "Shared.h"
#include "BaseModel.h"
#include "ImportedModel.h"
#include "PlaneMesh.h"
#include "Camera.h"
#include "Plane.h"
#include "ImGUIInterface.h"
#include <chrono>

class VisibilityBuffer :
	public VulkanWindow
{
public:

	VisibilityBuffer(Renderer* renderer, int width, int height)
		:VulkanWindow(renderer, width, height)
	{
		InitialiseVulkanApplication();
	}
	~VisibilityBuffer();
	struct uboVS {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	};
	struct vertice {
		VkPipelineVertexInputStateCreateInfo inputState;
		VkVertexInputBindingDescription bindingDescriptions;
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;
	};
	struct {
		vk::wrappers::ModelBuffers floor;
		vk::wrappers::ModelBuffers quad;
		vk::wrappers::ModelBuffers cube;
	}models;
	struct {
		VkDescriptorSet cube;
		VkDescriptorSet house;
	}descriptorSets;

	struct UboDataDynamic
	{
		glm::mat4 *model = nullptr;
	} uboDataDynamic;

	struct UboTextureDataDynamic
	{
		VkSampler *sampler = nullptr;
	} uboTextureDataDynamic;

	struct
	{
		VkDescriptorSetLayout IDLayout;
		VkDescriptorSetLayout triangleFilterLayout;
		VkDescriptorSetLayout compositionLayout;
	}descriptorSetLayouts;

	void GiveImGuiStaticInformation();
	void CreateImGui();
	void InitialiseVulkanApplication();
	void CreateCamera();
	void _CreateGeometry();
	void CreateVBuffer();
	void _CreateShaderBuffer(VkDevice device, VkDeviceSize size, VkBuffer * buffer, VkDeviceMemory* memory, VkBufferUsageFlagBits bufferStage, void* data);
	void _SetUpUniformBuffers();
	void UpdateUniformBuffer();
	void _CreateVIDCommandBuffers();
	void _CreateVertexFilteringCommandBuffers();
	void _CreateCommandBuffers() override;
	void _CreateVertexDescriptions();
	void _CreateDescriptorPool() override;
	void _CreateDescriptorSets() override;
	void _CreateDescriptorSetLayout() override;
	void Update() override;
	void DrawFrame() override;
	void _CreateGraphicsPipeline() override;


	ImGUIInterface *imGui = nullptr;

	ScreenTarget* screenTarget;
	PlaneMesh* planeMesh;
	ImportedModel* houseModel;

	size_t dynamicAlignment;
	size_t dynamicTextureAlignment;

	vk::wrappers::Buffer lightViewMatrixBuffer;
	glm::mat4 testLightViewMatrix;


	vk::wrappers::Buffer dynamicUboBuffer;
	vk::wrappers::Buffer dynamicTextureUboBuffer;

	vk::wrappers::Buffer sceneVertexBuffer;
	vk::wrappers::Buffer sceneIndexBuffer;
	std::vector<Vertex> sceneVertices;
	std::vector<uint32_t> sceneIndices;


	vertice vertices;
	uboVS IDMatricesVSData;
	vk::wrappers::Buffer IDMatricesUBOBuffer;
	vk::wrappers::VFrameBuffer IDFrameBuffer;
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	VkSampler colorSampler;


	VkCommandBuffer IDCmdBuffer = VK_NULL_HANDLE;

	VkDescriptorSet IDDescriptorSet;
	VkDescriptorSet compositionDescriptorSet;

	Camera* camera;

	VkSemaphore presentCompleteSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
	VkSemaphore offScreenSemaphore = VK_NULL_HANDLE;
	VkSemaphore IDPassSemaphore = VK_NULL_HANDLE;




	double frameTimeMRT;
	double frameTimeShading;

	int glfwOldKey, glfwNewKey;
	bool cameraUpdate = false;

};

