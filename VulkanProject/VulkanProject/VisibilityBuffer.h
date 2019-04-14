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

#define TEX_DIMENSIONS 2048
#define OFFSCREEN_WIDTH 1920
#define OFFSCREEN_HEIGHT 1080

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

	struct materialInfo {
		uint32_t materialID;
		uint32_t padding1;
	};
	
	struct vertexArgs {
		uint32_t vertexBase;
		uint32_t padding;
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


	void CreateShadowRenderPass();
	void CreateQueryPools();
	void GiveImGuiStaticInformation();
	void PrepareIndirectData();
	void CreateImGui();
	void InitialiseVulkanApplication();
	void CreateCamera();
	void _CreateGeometry();
	void CreateVBuffer();
	void _CreateShaderBuffer(VkDevice device, VkDeviceSize size, VkBuffer * buffer, VkDeviceMemory* memory, VkBufferUsageFlagBits bufferStage, void* data);
	void _SetUpUniformBuffers();
	void UpdateUniformBuffer();
	void _CreateShadowCommandBuffers();
	void _CreateVIDCommandBuffers();
	void _CreateVertexFilteringCommandBuffers();
	void _CreateOffScreenColorRenderPass();
	void _CreateOffScreenColorPipeline();
	void _CreateOffScreenColorCommandBuffers();
	void _CreateCommandBuffers() override;
	void _CreateVertexDescriptions();
	void _CreateDescriptorPool() override;
	void _CreateDescriptorSets() override;
	void _CreateDescriptorSetLayout() override;
	void Update() override;
	void DrawFrame() override;
	void _CreateGraphicsPipeline() override;
	void _CreateShadowPipeline();

	vk::wrappers::OSColorFrameBuffer offScreenColorFrameBuffer;

	ImGUIInterface *imGui = nullptr;

	ScreenTarget* screenTarget;
	PlaneMesh* planeMesh;
	ImportedModel* houseModel;

	size_t dynamicAlignment;
	size_t dynamicTextureAlignment;

	vk::wrappers::Buffer lightViewMatrixBuffer;
	glm::mat4 testLightViewMatrix;

	vk::wrappers::Buffer indirectCommandsBuffer;
	vk::wrappers::Buffer dynamicUboBuffer;
	vk::wrappers::Buffer dynamicTextureUboBuffer;

	vk::wrappers::Buffer sceneVertexBuffer;
	vk::wrappers::Buffer sceneIndexBuffer;

	vk::wrappers::Buffer materialIDBuffer;
	vk::wrappers::Buffer vertexStartBuffer;

	vk::wrappers::Buffer inverseVPBuffer;

	VkQueryPool VBIDQueryPool;
	VkQueryPool VBShadeQueryPool;

	//void* vbIDData;
	uint32_t VBIDQueryResultStart;
	uint32_t VBIDQueryResultEnd;

	double timeElapsedTest;
	double VBIDResultNanoSecondsStart;
	double VBIDResultNanoSecondsEnd;

	std::vector<Vertex> sceneVertices;
	std::vector<uint32_t> sceneIndices;

	VkDescriptorSetLayout shadowDescriptorSetLayout;
	VkDescriptorSet shadowDescriptorSet;
	vk::wrappers::ShadowFrameBuffer shadowFrameBuffer;

	vertice vertices;
	uboVS IDMatricesVSData;
	vk::wrappers::Buffer IDMatricesUBOBuffer;
	vk::wrappers::VFrameBuffer IDFrameBuffer;
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);


	VkSampler colorSampler;
	VkSampler shadowSampler;

	VkCommandBuffer IDCmdBuffer = VK_NULL_HANDLE;
	VkCommandBuffer shadowCmdBuffer = VK_NULL_HANDLE;
	VkCommandBuffer offScreenColorCmdBuffers = VK_NULL_HANDLE;

	VkDescriptorSet IDDescriptorSet;
	VkDescriptorSet compositionDescriptorSet;

	Camera* camera;

	VkSemaphore presentCompleteSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
	VkSemaphore shadowSemaphore = VK_NULL_HANDLE;
	VkSemaphore offScreenSemaphore = VK_NULL_HANDLE;
	VkSemaphore IDPassSemaphore = VK_NULL_HANDLE;
	VkSemaphore offScreenColorSemaphore = VK_NULL_HANDLE;

	std::vector<VkDrawIndexedIndirectCommand> indirectCommands;
	std::vector<materialInfo> materialIDs;
	std::vector<vertexArgs> startVertex;

	uint32_t indirectDrawCount;
	uint32_t drawModeValue;


	glm::mat4 inverseVP;


	double frameTimeMRT;
	double frameTimeShading;

	int glfwOldKey, glfwNewKey;
	bool cameraUpdate = false;

};

