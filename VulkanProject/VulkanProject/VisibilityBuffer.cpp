#include "VisibilityBuffer.h"


void VisibilityBuffer::InitialiseVulkanApplication()
{
	PrepareScene();
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore));
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore));
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &shadowSemaphore));
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &offScreenColorSemaphore));
	VisibilityBuffer::CreateImGui();
	VisibilityBuffer::CreateCamera();
	VisibilityBuffer::_CreateGeometry();
	VisibilityBuffer::CreateQueryPools();
	VisibilityBuffer::CreateShadowRenderPass();
	VisibilityBuffer::_CreateOffScreenColorRenderPass();
	VisibilityBuffer::CreateVBuffer();
	VisibilityBuffer::PrepareIndirectData();
	VisibilityBuffer::_SetUpUniformBuffers(); 
	VisibilityBuffer::_CreateDescriptorSetLayout();
	VisibilityBuffer::_CreateVertexDescriptions();
	VisibilityBuffer::_CreateGraphicsPipeline();
	VisibilityBuffer::_CreateOffScreenColorPipeline();
	VisibilityBuffer::_CreateDescriptorPool();
	VisibilityBuffer::_CreateDescriptorSets();
	VisibilityBuffer::_CreateShadowCommandBuffers();
	VisibilityBuffer::_CreateVIDCommandBuffers();
	VisibilityBuffer::_CreateCommandBuffers();
	VisibilityBuffer::_CreateOffScreenColorCommandBuffers();
	VisibilityBuffer::GiveImGuiStaticInformation();
	VisibilityBuffer::Update();
}

void VisibilityBuffer::CreateShadowRenderPass()
{

	shadowFrameBuffer.width = TEX_DIMENSIONS;
	shadowFrameBuffer.height = TEX_DIMENSIONS;

	//Find Depth Format
	VkFormat DepthFormat;
	DepthFormat = _FindDepthFormat();

	_CreateAttachment(DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &shadowFrameBuffer.depth, shadowFrameBuffer);

	//Attachment descriptions for renderpass 
	std::array<VkAttachmentDescription, 1> attachmentDescs = {};
	for (uint32_t i = 0; i < 1; ++i)
	{
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}


	//formats
	attachmentDescs[0].format = shadowFrameBuffer.depth.format;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = nullptr;
	subpass.colorAttachmentCount = 0;
	subpass.pDepthStencilAttachment = &depthReference;

	//Create the render pass using the attachments and dependencies data
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = attachmentDescs.data();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = nullptr;

	//Create the render pass to go into the frameBuffer struct
	vk::tools::ErrorCheck(vkCreateRenderPass(_renderer->GetVulkanDevice(), &renderPassInfo, nullptr, &shadowFrameBuffer.renderPass));

	std::array<VkImageView, 1> attachments;
	attachments[0] = shadowFrameBuffer.depth.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = shadowFrameBuffer.renderPass;
	frameBufferCreateInfo.pAttachments = attachments.data();
	frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	frameBufferCreateInfo.width = shadowFrameBuffer.width;
	frameBufferCreateInfo.height = shadowFrameBuffer.height;
	frameBufferCreateInfo.layers = 1;
	vk::tools::ErrorCheck(vkCreateFramebuffer(_renderer->GetVulkanDevice(), &frameBufferCreateInfo, nullptr, &shadowFrameBuffer.frameBuffer));

	//Create color sampler to sample from the color attachments.
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
	samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 1.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vk::tools::ErrorCheck(vkCreateSampler(_renderer->GetVulkanDevice(), &samplerCreateInfo, nullptr, &shadowSampler));

}

void VisibilityBuffer::CreateQueryPools()
{
	VkQueryPoolCreateInfo VBIDQueryInfo = {};
	VBIDQueryInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	VBIDQueryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	VBIDQueryInfo.pNext = 0;
	VBIDQueryInfo.flags = 0;
	VBIDQueryInfo.queryCount = 2;

	vkCreateQueryPool(_renderer->GetVulkanDevice(), &VBIDQueryInfo, nullptr, &VBIDQueryPool);


	VkQueryPoolCreateInfo VBShadeQueryInfo = {};
	VBShadeQueryInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	VBShadeQueryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	VBShadeQueryInfo.pNext = 0;
	VBShadeQueryInfo.flags = 0;
	VBShadeQueryInfo.queryCount = 2;

	vkCreateQueryPool(_renderer->GetVulkanDevice(), &VBShadeQueryInfo, nullptr, &VBShadeQueryPool);

}

void VisibilityBuffer::GiveImGuiStaticInformation()
{
	for (int i = 0; i < _models.size(); i++)
	{
		imGui->uiSettings.indices += _models[i]->model->GetIndexCount();
		imGui->uiSettings.vertices += _models[i]->model->GetVertexCount();
		imGui->uiSettings.face += (_models[i]->model->GetIndexCount() / 3);
	}
}

void VisibilityBuffer::PrepareIndirectData()
{
	int32_t m = 0;

	startVertex.resize(_models[0]->model->parts.size());
	materialIDs.resize(_models[0]->model->parts.size());


	for (size_t i = 0; i < _models[0]->model->parts.size(); i++)
	{
		VkDrawIndexedIndirectCommand drawIndexedCmd = {};
		drawIndexedCmd.instanceCount = 1;
		drawIndexedCmd.firstInstance = 0;
		drawIndexedCmd.firstIndex = _models[0]->model->parts[i].indexBase;
		drawIndexedCmd.indexCount = _models[0]->model->parts[i].indexCount;

		indirectCommands.push_back(drawIndexedCmd);
		startVertex[i].vertexBase = _models[0]->model->parts[i].indexBase;
		materialIDs[i].materialID = _models[0]->model->parts[i].materialID;


		m++;
	}

	indirectDrawCount = indirectDrawCount = static_cast<uint32_t>(indirectCommands.size());


	_CreateShaderBuffer(_renderer->GetVulkanDevice(), indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand), &indirectCommandsBuffer.buffer, &indirectCommandsBuffer.memory, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, indirectCommands.data());

	/*
	
	For each part in the model (each model in the scene for sponza)
		Create a VkDrawIndexedInDirectCommand
		set the instance count to the object_instance count (
		Set the first instance to be m * object_instance_count 
		Set the first index to be the model parts index base
		set the index count to be the model parts index count
		Push back the index draw amount to a vector



	indirectDrawCount = vkDrawIndexedCommand size;


	Create a loop for each indirect command created
		increment the object count by one per index command

	
	Make the indirect command buffer through _CreateShaderBuffer() using VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

		
	*/
}

void VisibilityBuffer::CreateImGui()
{
	imGui = new ImGUIInterface(_renderer);
	imGui->init((float)_swapChainExtent.width, (float)_swapChainExtent.height, "Visibility Buffer Rendering", ImGuiMode::VisBufferMode);
	imGui->initResources(_renderPass, _renderer->GetVulkanGraphicsQueue());
}

void VisibilityBuffer::CreateCamera()
{
	camera = new Camera(glm::vec3(-2.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 60.0f, glm::vec2(_swapChainExtent.width, _swapChainExtent.height), 0.1f, 200.0f);
}

//Destructor
VisibilityBuffer::~VisibilityBuffer()
{

}

//Loads geometry and creates geometry buffers
void VisibilityBuffer::_CreateGeometry()
{
	screenTarget = new ScreenTarget();
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), screenTarget->GetVertexBufferSize(), &screenTarget->GetVertexBuffer()->buffer, &screenTarget->GetVertexBuffer()->memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, screenTarget->GetVertexData());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), screenTarget->GetIndexBufferSize(), &screenTarget->GetIndexBuffer()->buffer, &screenTarget->GetIndexBuffer()->memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, screenTarget->GetIndexData());
	screenTarget->GetIndexBuffer()->SetUpDescriptorSet();
	screenTarget->GetVertexBuffer()->SetUpDescriptorSet();

	planeMesh = new PlaneMesh();
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), planeMesh->GetVertexBufferSize(), &planeMesh->GetVertexBuffer()->buffer, &planeMesh->GetVertexBuffer()->memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, planeMesh->GetVertexData());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), planeMesh->GetIndexBufferSize(), &planeMesh->GetIndexBuffer()->buffer, &planeMesh->GetIndexBuffer()->memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, planeMesh->GetIndexData());
	planeMesh->GetIndexBuffer()->SetUpDescriptorSet();
	planeMesh->GetVertexBuffer()->SetUpDescriptorSet();




	//Loads the sponza scene
	std::vector<std::string> textureFilepaths;
	
	houseModel = new ImportedModel("Textures/Sponza/sponza.obj", true, "Textures/Sponza/", textureFilepaths);
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), houseModel->GetVertexBufferSize(), &houseModel->GetVertexBuffer()->buffer, &houseModel->GetVertexBuffer()->memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, houseModel->GetVertexData());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), houseModel->GetIndexBufferSize(), &houseModel->GetIndexBuffer()->buffer, &houseModel->GetIndexBuffer()->memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, houseModel->GetIndexData());
	houseModel->GetIndexBuffer()->SetUpDescriptorSet();
	houseModel->GetVertexBuffer()->SetUpDescriptorSet();

	//Crytek Sponza scene
	//BaseModel::LoadMeshFromFile("Textures/Sponza/sponza.obj", "Textures/Sponza/", true, _meshes, textureFilepaths);
	


	//San_Miguel scene
	//BaseModel::LoadMeshFromFile("Textures/San_Miguel/san-miguel-low-poly.obj", "Textures/San_Miguel/san-miguel-low-poly.mtl", true, _meshes, textureFilepaths);
	
	for (int i = 0; i < textureFilepaths.size(); i++)
	{
		_textures.push_back(vk::wrappers::Texture2D());
		_textures[i].image = _CreateTextureImage(textureFilepaths[i].c_str());
		_textures[i].imageView = _CreateTextureImageView(_textures[i].image);
		_textures[i].sampler = _CreateTextureSampler();
	}
	
	_models.push_back(new vk::wrappers::Model());
	_models[0]->model = houseModel;


	
	//for (int i = 0; i < _meshes.size(); i++)
	//{
	//	_CreateShaderBuffer(_renderer->GetVulkanDevice(), _meshes[i]->GetVertexBufferSize(), &_meshes[i]->GetVertexBuffer()->buffer, &_meshes[i]->GetVertexBuffer()->memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, _meshes[i]->GetVertexData());
	//	_CreateShaderBuffer(_renderer->GetVulkanDevice(), _meshes[i]->GetIndexBufferSize(), &_meshes[i]->GetIndexBuffer()->buffer, &_meshes[i]->GetIndexBuffer()->memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, _meshes[i]->GetIndexData());
	//	_meshes[i]->GetIndexBuffer()->SetUpDescriptorSet();
	//	_meshes[i]->GetVertexBuffer()->SetUpDescriptorSet();
	//
	//	_models.push_back(new vk::wrappers::Model());
	//	_models[i]->model = _meshes[i];
	//
	//	sceneVertices.reserve(sceneVertices.size() + _meshes[i]->vertices.size());
	//	sceneIndices.reserve(sceneIndices.size() + _meshes[i]->indices.size());
	//	sceneVertices.insert(sceneVertices.end(), _meshes[i]->vertices.begin(), _meshes[i]->vertices.end());
	//	sceneIndices.insert(sceneIndices.end(), _meshes[i]->indices.begin(), _meshes[i]->indices.end());
	//}




	//*Lighting*
	_spotLights.push_back(new vk::wrappers::SpotLight());
	_pointLights.push_back(new vk::wrappers::PointLight());
	_directionalLights.push_back(new vk::wrappers::DirectionalLight());

	_directionalLights[0]->diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	_directionalLights[0]->direction = glm::vec4(2.0f, 7.0f, 2.0f, 1.0f);
	_directionalLights[0]->specular = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

	//Shadow matrices
	glm::mat4 testLightViewMatrix = glm::mat4(1.0f);
	testLightViewMatrix = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, -30.0f, 30.0f);
	glm::mat4 perspectView = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 60.0f);
	glm::mat4 lightView = glm::mat4(1.0f);
	lightView = glm::lookAt(glm::vec3(2.0f, 7.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 finalLightMatrix = testLightViewMatrix * lightView * glm::mat4(1.0f);



	_CreateShaderBuffer(_renderer->GetVulkanDevice(), sizeof(glm::mat4), &lightViewMatrixBuffer.buffer, &lightViewMatrixBuffer.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &finalLightMatrix);

	//Create lights storage buffers to pass to the fragment shader
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), _spotLights.size() * sizeof(vk::wrappers::SpotLight), &_spotLightBuffer, &_spotLightBufferMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, *_spotLights.data());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), _pointLights.size() * sizeof(vk::wrappers::PointLight), &_pointLightBuffer, &_pointLightBufferMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, *_pointLights.data());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), _directionalLights.size() * sizeof(vk::wrappers::DirectionalLight), &_directionalLightBuffer, &_directionalLightBufferMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, *_directionalLights.data());


	//_models.push_back(new vk::wrappers::Model());
	//_models[0]->model = houseModel;
	//_models[0]->texture.image = _CreateTextureImage("Textures/chalet.jpg");
	//_models[0]->texture.imageView = _CreateTextureImageView(_models[0]->texture.image);
	//_models[0]->texture.sampler = _CreateTextureSampler();
}

//Method for creating a memory buffer for shaders
//*Performs the staging buffer process to move to GPU memory
void VisibilityBuffer::_CreateShaderBuffer(VkDevice device, VkDeviceSize size, VkBuffer * buffer, VkDeviceMemory* memory, VkBufferUsageFlagBits bufferStage, void* data)
{
	VkDeviceSize bufferSize = size;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* tempData;
	vkMapMemory(_renderer->GetVulkanDevice(), stagingBufferMemory, 0, bufferSize, 0, &tempData);
	memcpy(tempData, data, (size_t)bufferSize);
	vkUnmapMemory(_renderer->GetVulkanDevice(), stagingBufferMemory);

	_CreateBuffer(bufferSize, bufferStage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, *buffer, *memory);
	_CopyBuffer(stagingBuffer, *buffer, bufferSize);

	vkDestroyBuffer(_renderer->GetVulkanDevice(), stagingBuffer, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), stagingBufferMemory, nullptr);
}

//Create the graphics pipelines in the renderer
void VisibilityBuffer::_CreateGraphicsPipeline()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.flags = 0;
	rasterizerCreateInfo.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = 0xf;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo blendStateInfo = {};
	blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendStateInfo.attachmentCount = 1;
	blendStateInfo.pAttachments = &colorBlendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;
	depthStencilState.front = {};

	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.flags = 0;

	VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {};
	multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateInfo.flags = 0;
	multisampleStateInfo.pSampleMask = 0;

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
	dynamicStateInfo.flags = 0;


	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::vbShade];
	pipelineCreateInfo.renderPass = _renderPass;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pColorBlendState = &blendStateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();

	VkSpecializationMapEntry specializationEntry{};
	specializationEntry.constantID = 0;
	specializationEntry.offset = 0;
	specializationEntry.size = sizeof(uint32_t);

	uint32_t specializationData = sampleCount;

	VkSpecializationInfo specializationInfo;
	specializationInfo.mapEntryCount = 1;
	specializationInfo.pMapEntries = &specializationEntry;
	specializationInfo.dataSize = sizeof(specializationData);
	specializationInfo.pData = &specializationData;


	//Final Pipeline (after offscreen pass)
	//Shader loading (Loads shader modules for pipeline)
	shaderStages[0] = loadShader("Shaders/VisibilityBuffer/VBShade.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/VisibilityBuffer/VBShade.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	shaderStages[1].pSpecializationInfo = &specializationInfo;

	VkPipelineVertexInputStateCreateInfo emptyVertexInputState = {};
	emptyVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
	multisampleStateInfo.rasterizationSamples = sampleCount;
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::vbShade];

	//Final composition graphics pipeline
	vk::tools::ErrorCheck(vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipelines[PipelineType::vbShade]));

	//Visibility buffer ID graphics pipeline
	//=========================
	shaderStages[0] = loadShader("Shaders/VisibilityBuffer/VBID.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/VisibilityBuffer/VBID.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.pVertexInputState = &vertices.inputState;

	multisampleStateInfo.rasterizationSamples = sampleCount;
	multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;


	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	////Swap the render pass and layout to the deferred renderpass and layout (offscreen rendering)
	pipelineCreateInfo.renderPass = IDFrameBuffer.renderPass;
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::vbID];
	//

	std::array<VkPipelineColorBlendAttachmentState, 1> blendAttachmentStates = {};
	VkPipelineColorBlendAttachmentState attachmentState = {};
	attachmentState.colorWriteMask = 0xf;
	attachmentState.blendEnable = VK_FALSE;
	blendAttachmentStates = { attachmentState };

	////Blend attachments for the pipeline
	blendStateInfo.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
	blendStateInfo.pAttachments = blendAttachmentStates.data();

	vk::tools::ErrorCheck(vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipelines[PipelineType::vbID]));

	_CreateShadowPipeline();
}

//Creates the shadow map pipeline for shadow rendering
void VisibilityBuffer::_CreateShadowPipeline()
{

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.flags = 0;
	rasterizerCreateInfo.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = 0xf;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo blendStateInfo = {};
	blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendStateInfo.attachmentCount = 1;
	blendStateInfo.pAttachments = &colorBlendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;
	depthStencilState.front = {};



	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.flags = 0;

	VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {};
	multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateInfo.flags = 0;
	multisampleStateInfo.pSampleMask = 0;

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
	dynamicStateInfo.flags = 0;


	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::shadowMap];
	pipelineCreateInfo.renderPass = shadowFrameBuffer.renderPass;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pColorBlendState = &blendStateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();


	//Final Pipeline (after offscreen pass)
	//Shader loading (Loads shader modules for pipeline)
	shaderStages[0] = loadShader("Shaders/Deferred/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/Deferred/shadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineVertexInputStateCreateInfo emptyVertexInputState = {};
	emptyVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	pipelineCreateInfo.pVertexInputState = &vertices.inputState;

	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::shadowMap];

	vk::tools::ErrorCheck(vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipelines[PipelineType::shadowMap]));
}

//Creates and sets up the uniform buffers for the shader passes
void VisibilityBuffer::_SetUpUniformBuffers()
{
	//Ubo alignment for dynamic buffer model matrices
	size_t minUboAlignment = _renderer->GetVulkanPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	dynamicAlignment = sizeof(glm::mat4);
	if (minUboAlignment > 0)
	{
		dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	//checks buffer size by how many models there are times alignment
	size_t bufferSize = _models.size() * dynamicAlignment;
	uboDataDynamic.model = (glm::mat4*)_aligned_malloc(bufferSize, dynamicAlignment);
	assert(uboDataDynamic.model);


	std::cout << "MinUniformBufferOffsetAlignment = " << minUboAlignment << std::endl;
	std::cout << "Dynamic alignment = " << dynamicAlignment << std::endl;

	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, dynamicUboBuffer.buffer, dynamicUboBuffer.memory);
	dynamicUboBuffer.SetUpDescriptorSet();


	//loop through each model
	for (uint32_t i = 0; i < _models.size(); i++)
	{
		glm::mat4* modelMat = (glm::mat4*)(((uint64_t)uboDataDynamic.model + (i * dynamicAlignment)));
		VkSampler* modelSampler = (VkSampler*)(((uint64_t)uboTextureDataDynamic.sampler + (i * dynamicTextureAlignment)));


		if (i == 0)
		{
			*modelMat = glm::mat4(1.0f);
			*modelMat = glm::translate(*modelMat, glm::vec3(0.0f, -2.0f, 0.0f));
			//*modelMat = glm::rotate(*modelMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			*modelMat = glm::scale(*modelMat, glm::vec3(0.005f, 0.005f, 0.005f));
		}
		//else if (i == 1)
		//{
		//	*modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
		//	*modelMat = glm::rotate(*modelMat, glm::radians(-180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//	*modelMat = glm::scale(*modelMat, glm::vec3(4.0f, 4.0f, 4.0f));
		//}
	}


	VisibilityBuffer::_CreateShaderBuffer(_renderer->GetVulkanDevice(), static_cast<uint32_t>(materialIDs.size()) * sizeof(materialInfo), &materialIDBuffer.buffer, &materialIDBuffer.memory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, materialIDs.data());
	VisibilityBuffer::_CreateShaderBuffer(_renderer->GetVulkanDevice(), static_cast<uint32_t>(startVertex.size()) * sizeof(vertexArgs), &vertexStartBuffer.buffer, &vertexStartBuffer.memory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, startVertex.data());


	//Creates dynamic uniform buffer
	//_CreateShaderBuffer(_renderer->GetVulkanDevice(), bufferSize, &dynamicUboBuffer.buffer, &dynamicUboBuffer.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &uboDataDynamic);
	//Maps the data to the dynamic uniform buffer
	void* data2;
	vkMapMemory(_renderer->GetVulkanDevice(), dynamicUboBuffer.memory, 0, bufferSize, 0, &data2);
	memcpy(data2, uboDataDynamic.model, bufferSize);
	dynamicUboBuffer.SetUpDescriptorSet();

	//Vertex UBO
	VisibilityBuffer::_CreateShaderBuffer(_renderer->GetVulkanDevice(), sizeof(uboVS), &IDMatricesUBOBuffer.buffer, &IDMatricesUBOBuffer.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &IDMatricesVSData);
	IDMatricesVSData.model = glm::mat4(1.0f);
	IDMatricesVSData.model = glm::scale(IDMatricesVSData.model, glm::vec3(0.005f, 0.005f, 0.005f));
	IDMatricesVSData.view = camera->GetViewMatrix();
	IDMatricesVSData.projection = camera->GetProjectionMatrix();
	//IDMatricesVSData.projection[1][1] *= -1;

	glm::mat4 vp = IDMatricesVSData.projection * IDMatricesVSData.view;
	glm::mat4 tempInverseVp = glm::inverse(vp);

	inverseVP = tempInverseVp;
	VisibilityBuffer::_CreateShaderBuffer(_renderer->GetVulkanDevice(), sizeof(glm::mat4), &inverseVPBuffer.buffer, &inverseVPBuffer.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &inverseVP);


	void* data;
	vkMapMemory(_renderer->GetVulkanDevice(), IDMatricesUBOBuffer.memory, 0, sizeof(uboVS), 0, &data);
	memcpy(data, &IDMatricesVSData, sizeof(uboVS));
	IDMatricesUBOBuffer.SetUpDescriptorSet();
}

//Updates the uniform buffers for the descriptor sets
void VisibilityBuffer::UpdateUniformBuffer()
{
	IDMatricesVSData.view = camera->GetViewMatrix();
	IDMatricesVSData.projection = camera->GetProjectionMatrix();

	glm::mat4 vp = camera->GetProjectionMatrix() * camera->GetViewMatrix();
	glm::mat4 tempInverseVP = glm::inverse(vp);
	inverseVP = tempInverseVP;
	//inverseVP = vp;

	void* data;
	vkMapMemory(_renderer->GetVulkanDevice(), IDMatricesUBOBuffer.memory, 0, sizeof(uboVS), 0, &data);
	memcpy(data, &IDMatricesVSData, sizeof(uboVS));
	vkUnmapMemory(_renderer->GetVulkanDevice(), IDMatricesUBOBuffer.memory);
	IDMatricesUBOBuffer.SetUpDescriptorSet();


	void* data2;
	vkMapMemory(_renderer->GetVulkanDevice(), inverseVPBuffer.memory, 0, sizeof(glm::mat4), 0, &data2);
	memcpy(data2, &inverseVP, sizeof(glm::mat4));
	vkUnmapMemory(_renderer->GetVulkanDevice(), inverseVPBuffer.memory);



}

void VisibilityBuffer::_CreateShadowCommandBuffers()
{
	//if the offscreen cmd buffer hasn't been initialised.
	if (shadowCmdBuffer == VK_NULL_HANDLE)
	{
		//Create a single command buffer for the offscreen rendering
		VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.commandPool = _commandPool;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate_info.commandBufferCount = 1;

		vkAllocateCommandBuffers(_renderer->GetVulkanDevice(), &command_buffer_allocate_info, &shadowCmdBuffer);
	}

	//Set up semaphore create info
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	//Create a signal semaphore for when the off screen rendering is complete (For pipeline ordering)
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &shadowSemaphore));

	//set up cmd buffer begin info
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	//Clear values for attachments in fragment shader
	std::array<VkClearValue, 1> clearValues;
	clearValues[0].depthStencil = { 1.0f, 0 };

	//begin to set up the information for the render pass
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.framebuffer = shadowFrameBuffer.frameBuffer;
	renderPassBeginInfo.renderPass = shadowFrameBuffer.renderPass;
	renderPassBeginInfo.renderArea.extent.width = shadowFrameBuffer.width;
	renderPassBeginInfo.renderArea.extent.height = shadowFrameBuffer.height;
	renderPassBeginInfo.clearValueCount = 0;
	renderPassBeginInfo.pClearValues = nullptr;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();


	//begin command buffer and start the render pass
	vk::tools::ErrorCheck(vkBeginCommandBuffer(shadowCmdBuffer, &cmdBufferBeginInfo));
	vkCmdBeginRenderPass(shadowCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)shadowFrameBuffer.width;
	viewport.height = (float)shadowFrameBuffer.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(shadowCmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.width = shadowFrameBuffer.width;
	scissor.extent.height = shadowFrameBuffer.height;
	scissor.offset = { 0,0 };
	vkCmdSetScissor(shadowCmdBuffer, 0, 1, &scissor);
	vkCmdBindPipeline(shadowCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::shadowMap]);
	VkDeviceSize offsets[1] = { 0 };

	//Loop for each model in our scene
	for (size_t i = 0; i < _models.size(); i++)
	{
		//Dynamic offset to get the correct model matrix from the dynamic buffer
		uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);
		//binding descriptor sets and drawing model on the screen
		vkCmdBindDescriptorSets(shadowCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::shadowMap], 0, 1, &shadowDescriptorSet, 1, &dynamicOffset);
		vkCmdBindVertexBuffers(shadowCmdBuffer, 0, 1, &_models[i]->model->GetVertexBuffer()->buffer, offsets);
		vkCmdBindIndexBuffer(shadowCmdBuffer, _models[i]->model->GetIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(shadowCmdBuffer, _models[i]->model->GetIndexCount(), 1, 0, 0, 0);
	}


	vkCmdEndRenderPass(shadowCmdBuffer);

	//End the command buffer drawing
	vk::tools::ErrorCheck(vkEndCommandBuffer(shadowCmdBuffer));

}

//Creates visibility buffer attachments for the first render pass 
void VisibilityBuffer::CreateVBuffer()
{
	IDFrameBuffer.width = OFFSCREEN_WIDTH;
	IDFrameBuffer.height = OFFSCREEN_HEIGHT;

	_CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &IDFrameBuffer.VID, IDFrameBuffer);
	//_CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &IDFrameBuffer.VID, IDFrameBuffer);
	
	//Find Depth Format
	VkFormat DepthFormat;
	DepthFormat = _FindDepthFormat();

	_CreateAttachment(DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &IDFrameBuffer.depth, IDFrameBuffer);

	//Attachment descriptions for renderpass 
	std::array<VkAttachmentDescription, 2> attachmentDescs = {};
	for (uint32_t i = 0; i < 2; i++)
	{
		attachmentDescs[i].samples = sampleCount;
		attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//if we're on the depth image description
		if (i == 1)
		{
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else
		{
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
	}
	//formats
	attachmentDescs[0].format = IDFrameBuffer.VID.format;
	attachmentDescs[1].format = IDFrameBuffer.depth.format;

	std::vector<VkAttachmentReference> colorReferences;
	colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = colorReferences.data();
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
	subpass.pDepthStencilAttachment = &depthReference;

	//Subpass dependencies
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	//What was supposed to happen : 
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
	//What actually happened:
	//dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	//Create the render pass using the attachments and dependencies data
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = attachmentDescs.data();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies.data();

	//Create the render pass to go into the frameBuffer struct
	vk::tools::ErrorCheck(vkCreateRenderPass(_renderer->GetVulkanDevice(), &renderPassInfo, nullptr, &IDFrameBuffer.renderPass));

	std::array<VkImageView, 2> attachments;
	attachments[0] = IDFrameBuffer.VID.view;
	attachments[1] = IDFrameBuffer.depth.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = IDFrameBuffer.renderPass;
	frameBufferCreateInfo.pAttachments = attachments.data();
	frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	frameBufferCreateInfo.width = IDFrameBuffer.width;
	frameBufferCreateInfo.height = IDFrameBuffer.height;
	frameBufferCreateInfo.layers = 1;
	vk::tools::ErrorCheck(vkCreateFramebuffer(_renderer->GetVulkanDevice(), &frameBufferCreateInfo, nullptr, &IDFrameBuffer.frameBuffer));


	//Create color sampler to sample from the color attachments.
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
	samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 1.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vk::tools::ErrorCheck(vkCreateSampler(_renderer->GetVulkanDevice(), &samplerCreateInfo, nullptr, &colorSampler));


}

//First pass for the visibility Buffer: Renders vertex ID values to a texture
//*First Pass for Visibility Buffer
void VisibilityBuffer::_CreateVIDCommandBuffers()
{
	//if the offscreen cmd buffer hasn't been initialised.
	if (IDCmdBuffer == VK_NULL_HANDLE)
	{
		//Create a single command buffer for the offscreen rendering
		VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.commandPool = _commandPool;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate_info.commandBufferCount = 1;

		vkAllocateCommandBuffers(_renderer->GetVulkanDevice(), &command_buffer_allocate_info, &IDCmdBuffer);
	}

	//Set up semaphore create info
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	//Create a signal semaphore for when the off screen rendering is complete (For pipeline ordering)
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &IDPassSemaphore));

	//set up cmd buffer begin info
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	//Clear values for attachments in fragment shader
	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = { { 0.0f,0.0f,0.0f,0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	//begin to set up the information for the render pass
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.framebuffer = IDFrameBuffer.frameBuffer;
	renderPassBeginInfo.renderPass = IDFrameBuffer.renderPass;
	renderPassBeginInfo.renderArea.extent.width = IDFrameBuffer.width;
	renderPassBeginInfo.renderArea.extent.height = IDFrameBuffer.height;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();


	//begin command buffer and start the render pass

	vk::tools::ErrorCheck(vkBeginCommandBuffer(IDCmdBuffer, &cmdBufferBeginInfo));
	vkCmdResetQueryPool(IDCmdBuffer, VBIDQueryPool, 0, 1);
	vkCmdResetQueryPool(IDCmdBuffer, VBIDQueryPool, 1, 1);

	vkCmdWriteTimestamp(IDCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VBIDQueryPool, 0);
	vkCmdBeginRenderPass(IDCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)IDFrameBuffer.width;
		viewport.height = (float)IDFrameBuffer.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(IDCmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.extent.width = IDFrameBuffer.width;
		scissor.extent.height = IDFrameBuffer.height;
		scissor.offset = { 0,0 };
		vkCmdSetScissor(IDCmdBuffer, 0, 1, &scissor);
	
		VkDeviceSize offsets[1] = { 0 };

		
		//Realistically, this should be the draw command vs the first draw command bellow
		vkCmdBindPipeline(IDCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::vbID]);
		vkCmdBindDescriptorSets(IDCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::vbID], 0, 1, &descriptorSets.house, 0, NULL);
		vkCmdBindVertexBuffers(IDCmdBuffer, 0, 1, &_models[0]->model->GetVertexBuffer()->buffer, offsets);
		vkCmdBindIndexBuffer(IDCmdBuffer, _models[0]->model->GetIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexedIndirect(IDCmdBuffer, indirectCommandsBuffer.buffer, 0, indirectDrawCount, sizeof(VkDrawIndexedIndirectCommand));
		
		//for (int i = 0; i < _models.size(); i++)
		//{
		//
		//	vkCmdPushConstants(IDCmdBuffer, _pipelineLayout[PipelineType::vbID], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &_models[i]->model->materialID);
		//	vkCmdBindPipeline(IDCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::vbID]);
		//	vkCmdBindDescriptorSets(IDCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::vbID], 0, 1, &descriptorSets.house, 0, NULL);
		//	vkCmdBindVertexBuffers(IDCmdBuffer, 0, 1, &_models[i]->model->GetVertexBuffer()->buffer, offsets);
		//	vkCmdBindIndexBuffer(IDCmdBuffer, _models[i]->model->GetIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
		//	vkCmdDrawIndexed(IDCmdBuffer, _models[i]->model->GetIndexCount(), 1, 0, 0, 0);
		//}
		
			//House
	

	vkCmdEndRenderPass(IDCmdBuffer);
	vkCmdWriteTimestamp(IDCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VBIDQueryPool, 1);
	//End the command buffer drawing
	vk::tools::ErrorCheck(vkEndCommandBuffer(IDCmdBuffer));

}

//Second pass for the visibility buffer: Filters triangles using culling methods to reduce triangle batches
//*Second pass for Visibility buffer
void VisibilityBuffer::_CreateVertexFilteringCommandBuffers()
{
}

void VisibilityBuffer::_CreateOffScreenColorRenderPass()
{

	offScreenColorFrameBuffer.width = OFFSCREEN_WIDTH;
	offScreenColorFrameBuffer.height = OFFSCREEN_HEIGHT;

	//Create the color attachments for each screen render
	_CreateAttachment(_swapChainImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offScreenColorFrameBuffer.color, offScreenColorFrameBuffer);

	//depth attachment

	//Find Depth Format
	VkFormat DepthFormat;
	DepthFormat = _FindDepthFormat();

	_CreateAttachment(DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &offScreenColorFrameBuffer.depth, offScreenColorFrameBuffer);


	//Attachment descriptions for renderpass 
	std::array<VkAttachmentDescription, 2> attachmentDescs = {};
	for (uint32_t i = 0; i < attachmentDescs.size(); ++i)
	{
		attachmentDescs[i].samples = sampleCount;
		attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//if we're on the depth image description
		if (i == 3)
		{
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else
		{
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
	}

	//formats
	attachmentDescs[0].format = offScreenColorFrameBuffer.color.format;
	attachmentDescs[1].format = offScreenColorFrameBuffer.depth.format;

	std::vector<VkAttachmentReference> colorReferences;
	colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = colorReferences.data();
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
	subpass.pDepthStencilAttachment = &depthReference;

	//Subpass dependencies
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	//Create the render pass using the attachments and dependencies data
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = attachmentDescs.data();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies.data();

	//Create the render pass to go into the frameBuffer struct
	vk::tools::ErrorCheck(vkCreateRenderPass(_renderer->GetVulkanDevice(), &renderPassInfo, nullptr, &offScreenColorFrameBuffer.renderPass));

	std::array<VkImageView, 2> attachments;
	attachments[0] = offScreenColorFrameBuffer.color.view;
	attachments[1] = offScreenColorFrameBuffer.depth.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = offScreenColorFrameBuffer.renderPass;
	frameBufferCreateInfo.pAttachments = attachments.data();
	frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	frameBufferCreateInfo.width = offScreenColorFrameBuffer.width;
	frameBufferCreateInfo.height = offScreenColorFrameBuffer.height;
	frameBufferCreateInfo.layers = 1;
	vk::tools::ErrorCheck(vkCreateFramebuffer(_renderer->GetVulkanDevice(), &frameBufferCreateInfo, nullptr, &offScreenColorFrameBuffer.frameBuffer));
}

void VisibilityBuffer::_CreateOffScreenColorPipeline()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.flags = 0;
	rasterizerCreateInfo.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = 0xf;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo blendStateInfo = {};
	blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendStateInfo.attachmentCount = 1;
	blendStateInfo.pAttachments = &colorBlendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;
	depthStencilState.front = {};



	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.flags = 0;

	VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {};
	multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateInfo.flags = 0;
	multisampleStateInfo.pSampleMask = 0;

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
	dynamicStateInfo.flags = 0;


	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::vbShade];
	pipelineCreateInfo.renderPass = offScreenColorFrameBuffer.renderPass;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pColorBlendState = &blendStateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();

	VkSpecializationMapEntry specializationEntry{};
	specializationEntry.constantID = 0;
	specializationEntry.offset = 0;
	specializationEntry.size = sizeof(uint32_t);

	uint32_t specializationData = sampleCount;

	VkSpecializationInfo specializationInfo;
	specializationInfo.mapEntryCount = 1;
	specializationInfo.pMapEntries = &specializationEntry;
	specializationInfo.dataSize = sizeof(specializationData);
	specializationInfo.pData = &specializationData;


	//Final Pipeline (after offscreen pass)
	//Shader loading (Loads shader modules for pipeline)
	shaderStages[0] = loadShader("Shaders/VisibilityBuffer/VBShade.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/VisibilityBuffer/VBShade.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	shaderStages[1].pSpecializationInfo = &specializationInfo;

	VkPipelineVertexInputStateCreateInfo emptyVertexInputState = {};
	emptyVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
	multisampleStateInfo.rasterizationSamples = sampleCount;
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::vbShade];

	vk::tools::ErrorCheck(vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipelines[PipelineType::offscreenColor]));
}

void VisibilityBuffer::_CreateOffScreenColorCommandBuffers()
{

	if (offScreenColorCmdBuffers == VK_NULL_HANDLE)
	{
		VkCommandBufferAllocateInfo command_buffer_allocate_info{};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.commandPool = _commandPool;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate_info.commandBufferCount = 1;

		vk::tools::ErrorCheck(vkAllocateCommandBuffers(_renderer->GetVulkanDevice(), &command_buffer_allocate_info, &offScreenColorCmdBuffers));
	}


	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = offScreenColorFrameBuffer.renderPass;
	render_pass_begin_info.renderArea.extent.width = offScreenColorFrameBuffer.width;
	render_pass_begin_info.renderArea.extent.height = offScreenColorFrameBuffer.height;
	render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clearValues.size());
	render_pass_begin_info.pClearValues = clearValues.data();
	render_pass_begin_info.framebuffer = offScreenColorFrameBuffer.frameBuffer;


	vk::tools::ErrorCheck(vkBeginCommandBuffer(offScreenColorCmdBuffers, &command_buffer_begin_info));
	vkCmdResetQueryPool(offScreenColorCmdBuffers, VBShadeQueryPool, 0, 1);
	vkCmdResetQueryPool(offScreenColorCmdBuffers, VBShadeQueryPool, 1, 1);
	vkCmdWriteTimestamp(offScreenColorCmdBuffers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VBShadeQueryPool, 0); // Write query #1
	vkCmdBeginRenderPass(offScreenColorCmdBuffers, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)offScreenColorFrameBuffer.width;
	viewport.height = (float)offScreenColorFrameBuffer.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(offScreenColorCmdBuffers, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.width = offScreenColorFrameBuffer.width;
	scissor.extent.height = offScreenColorFrameBuffer.height;
	scissor.offset = { 0,0 };
	vkCmdSetScissor(offScreenColorCmdBuffers, 0, 1, &scissor);


	VkDeviceSize offsets[] = { 0 };


	vkCmdPushConstants(offScreenColorCmdBuffers, _pipelineLayout[PipelineType::vbShade], VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &drawModeValue);
	vkCmdBindPipeline(offScreenColorCmdBuffers, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::offscreenColor]);
	vkCmdBindDescriptorSets(offScreenColorCmdBuffers, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::vbShade], 0, 1, &compositionDescriptorSet, 0, NULL);
	vkCmdDraw(offScreenColorCmdBuffers, 3, 1, 0, 0);

	vkCmdEndRenderPass(offScreenColorCmdBuffers);
	//vkCmdEndRenderPass(_drawCommandBuffers[i]);
	vkCmdWriteTimestamp(offScreenColorCmdBuffers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VBShadeQueryPool, 1); // Write query #2

	vk::tools::ErrorCheck(vkEndCommandBuffer(offScreenColorCmdBuffers));
}

//Third Pass for visibility Buffer: Gathers Vertex ID values to redraw the scene
//*Final Pass for visibility Buffer
void VisibilityBuffer::_CreateCommandBuffers()
{
	if (_drawCommandBuffers.size() != _swapChainFramebuffers.size())
	{
		_drawCommandBuffers.resize(_swapChainFramebuffers.size());
		_drawCommandBuffers[0] = VK_NULL_HANDLE;
		_drawCommandBuffers[1] = VK_NULL_HANDLE;
		_drawCommandBuffers[2] = VK_NULL_HANDLE;	
	}

	if (_drawCommandBuffers[0] == VK_NULL_HANDLE)
	{
		VkCommandBufferAllocateInfo command_buffer_allocate_info{};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.commandPool = _commandPool;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate_info.commandBufferCount = static_cast<uint32_t>(_drawCommandBuffers.size());

		vk::tools::ErrorCheck(vkAllocateCommandBuffers(_renderer->GetVulkanDevice(), &command_buffer_allocate_info, _drawCommandBuffers.data()));
	}

	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = _renderPass;
	render_pass_begin_info.renderArea.extent.width = _swapChainExtent.width;
	render_pass_begin_info.renderArea.extent.height = _swapChainExtent.height;
	render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clearValues.size());
	render_pass_begin_info.pClearValues = clearValues.data();

	imGui->newFrame(frameTimeMRT, frameTimeShading, (currentFrame == 0));
	imGui->updateBuffers();

	//Use swapchain draw command buffers to draw the scene
	for (size_t i = 0; i < _drawCommandBuffers.size(); i++)
	{
		render_pass_begin_info.framebuffer = _swapChainFramebuffers[i];

		vk::tools::ErrorCheck(vkBeginCommandBuffer(_drawCommandBuffers[i], &command_buffer_begin_info));
		vkCmdBeginRenderPass(_drawCommandBuffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)IDFrameBuffer.width;
			viewport.height = (float)IDFrameBuffer.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(_drawCommandBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.extent.width = IDFrameBuffer.width;
			scissor.extent.height = IDFrameBuffer.height;
			scissor.offset = { 0,0 };
			vkCmdSetScissor(_drawCommandBuffers[i], 0, 1, &scissor);
			VkDeviceSize offsets[] = { 0 };

			vkCmdPushConstants(_drawCommandBuffers[i], _pipelineLayout[PipelineType::vbShade], VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &drawModeValue);
			vkCmdBindPipeline(_drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::vbShade]);
			vkCmdBindDescriptorSets(_drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::vbShade], 0, 1, &compositionDescriptorSet, 0, NULL);
			vkCmdDraw(_drawCommandBuffers[i],3, 1, 0, 0);

			imGui->DrawFrame(_drawCommandBuffers[i]);

		vkCmdEndRenderPass(_drawCommandBuffers[i]);

		vk::tools::ErrorCheck(vkEndCommandBuffer(_drawCommandBuffers[i]));
	}
}

//Set up the vertex descriptions for rendering geometry in 3D space (Position, Color and UV)
void VisibilityBuffer::_CreateVertexDescriptions()
{
	vertices.bindingDescriptions.binding = 0;
	vertices.bindingDescriptions.stride = sizeof(Vertex);
	vertices.bindingDescriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertices.attributeDescriptions[0].binding = 0;
	vertices.attributeDescriptions[0].location = 0;
	vertices.attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertices.attributeDescriptions[0].offset = offsetof(Vertex, pos);

	vertices.attributeDescriptions[1].binding = 0;
	vertices.attributeDescriptions[1].location = 1;
	vertices.attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertices.attributeDescriptions[1].offset = offsetof(Vertex, color);


	vertices.inputState = {};
	vertices.inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertices.inputState.flags = 0;
	vertices.inputState.pNext = 0;
	vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.attributeDescriptions.size());
	vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	vertices.inputState.vertexBindingDescriptionCount = 1;
	vertices.inputState.pVertexBindingDescriptions = &vertices.bindingDescriptions;
}

//Set up descriptor pool for descriptor layouts
void VisibilityBuffer::_CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 3> pool_sizes{};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(100);
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = static_cast<uint32_t>(100);
	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[2].descriptorCount = static_cast<uint32_t>(10);

	VkDescriptorPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_create_info.pPoolSizes = pool_sizes.data();
	pool_create_info.maxSets = static_cast<uint32_t>(100);
	


	vk::tools::ErrorCheck(vkCreateDescriptorPool(_renderer->GetVulkanDevice(), &pool_create_info, nullptr, &_descriptorPool));
}

//Sets up the descriptor sets for each pipeline in the renderer
void VisibilityBuffer::_CreateDescriptorSets()
{
	//For rendering the quad
	VkDescriptorSetAllocateInfo descSetAllocInfo1{};
	descSetAllocInfo1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo1.descriptorPool = _descriptorPool;
	descSetAllocInfo1.descriptorSetCount = 1;
	descSetAllocInfo1.pSetLayouts = &descriptorSetLayouts.IDLayout;

	//OffScreen (scene)
	vk::tools::ErrorCheck(vkAllocateDescriptorSets(_renderer->GetVulkanDevice(), &descSetAllocInfo1, &descriptorSets.house));

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = IDMatricesUBOBuffer.buffer;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(uboVS);

	std::vector<VkWriteDescriptorSet> descriptor_writes_model;
	//Binding 0: Vertex Shader UBO
	descriptor_writes_model.resize(1);
	descriptor_writes_model[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes_model[0].dstSet = descriptorSets.house;
	descriptor_writes_model[0].dstBinding = 0;
	descriptor_writes_model[0].dstArrayElement = 0;
	descriptor_writes_model[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes_model[0].descriptorCount = 1;
	descriptor_writes_model[0].pImageInfo = 0;
	descriptor_writes_model[0].pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(_renderer->GetVulkanDevice(), static_cast<uint32_t>(descriptor_writes_model.size()), descriptor_writes_model.data(), 0, nullptr);

	//For rendering the quad
	VkDescriptorSetAllocateInfo descSetAllocInfo2 = {};
	descSetAllocInfo2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo2.descriptorPool = _descriptorPool;
	descSetAllocInfo2.descriptorSetCount = 1;
	descSetAllocInfo2.pSetLayouts = &descriptorSetLayouts.compositionLayout;



	vk::tools::ErrorCheck(vkAllocateDescriptorSets(_renderer->GetVulkanDevice(), &descSetAllocInfo2, &compositionDescriptorSet));

	VkDescriptorBufferInfo ubo_buffer_info = {};
	ubo_buffer_info.buffer = IDMatricesUBOBuffer.buffer;
	ubo_buffer_info.offset = 0;
	ubo_buffer_info.range = sizeof(uboVS);

	uint32_t arraySize = _textures.size();

	std::vector<VkDescriptorImageInfo> diffuse_array_image_info = {};
	diffuse_array_image_info.resize(static_cast<uint32_t>(_textures.size()));

	for (size_t i = 0; i < diffuse_array_image_info.size(); i++)
	{
		diffuse_array_image_info[i].sampler = _textures[i].sampler;
		diffuse_array_image_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		diffuse_array_image_info[i].imageView = _textures[i].imageView;
	}

	VkDescriptorImageInfo vbid_image_info = {};
	vbid_image_info.sampler = colorSampler;
	vbid_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vbid_image_info.imageView = IDFrameBuffer.VID.view;

	VkDescriptorBufferInfo index_buffer_info = {};
	index_buffer_info.buffer = _models[0]->model->GetIndexBuffer()->buffer;
	index_buffer_info.offset = 0;
	index_buffer_info.range = _models[0]->model->GetIndexBufferSize();

	VkDescriptorBufferInfo vertex_position_info = {};
	vertex_position_info.buffer = _models[0]->model->GetVertexBuffer()->buffer;
	vertex_position_info.offset = 0;
	vertex_position_info.range = _models[0]->model->GetVertexBufferSize();


	VkDescriptorBufferInfo start_vertex_info = {};
	start_vertex_info.buffer = vertexStartBuffer.buffer;
	start_vertex_info.offset = 0;
	start_vertex_info.range = static_cast<uint32_t>(startVertex.size()) * sizeof(vertexArgs);


	VkDescriptorBufferInfo material_data_info = {};
	material_data_info.buffer = materialIDBuffer.buffer;
	material_data_info.offset = 0;
	material_data_info.range = static_cast<uint32_t>(materialIDs.size()) * sizeof(materialInfo);

	VkDescriptorBufferInfo directional_light_info = {};
	directional_light_info.buffer = _directionalLightBuffer;
	directional_light_info.offset = 0;
	directional_light_info.range = static_cast<uint32_t>(_directionalLights.size()) * sizeof(vk::wrappers::DirectionalLight);

	VkDescriptorImageInfo shadow_map_image_info = {};
	shadow_map_image_info.sampler = shadowSampler;
	shadow_map_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadow_map_image_info.imageView = shadowFrameBuffer.depth.view;


	VkDescriptorBufferInfo inverse_matrix_buffer_info = {};
	inverse_matrix_buffer_info.buffer = inverseVPBuffer.buffer;
	inverse_matrix_buffer_info.offset = 0;
	inverse_matrix_buffer_info.range = sizeof(glm::mat4);

	VkDescriptorBufferInfo light_matrix_shade_buffer_info = {};
	light_matrix_shade_buffer_info.buffer = lightViewMatrixBuffer.buffer;
	light_matrix_shade_buffer_info.offset = 0;
	light_matrix_shade_buffer_info.range = sizeof(glm::mat4);


	std::vector<VkWriteDescriptorSet> compDescriptorWritesModel;
	//Binding 0: Vertex Shader UBO
	compDescriptorWritesModel.resize(11);
	compDescriptorWritesModel[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[0].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[0].dstBinding = 0;
	compDescriptorWritesModel[0].dstArrayElement = 0;
	compDescriptorWritesModel[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	compDescriptorWritesModel[0].descriptorCount = 1;
	compDescriptorWritesModel[0].pImageInfo = 0;
	compDescriptorWritesModel[0].pBufferInfo = &ubo_buffer_info;

	compDescriptorWritesModel[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[1].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[1].dstBinding = 1;
	compDescriptorWritesModel[1].dstArrayElement = 0;
	compDescriptorWritesModel[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	compDescriptorWritesModel[1].descriptorCount = static_cast<uint32_t>(diffuse_array_image_info.size());
	compDescriptorWritesModel[1].pImageInfo = diffuse_array_image_info.data();
	compDescriptorWritesModel[1].pBufferInfo = 0;

	compDescriptorWritesModel[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[2].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[2].dstBinding = 2;
	compDescriptorWritesModel[2].dstArrayElement = 0;
	compDescriptorWritesModel[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	compDescriptorWritesModel[2].descriptorCount = 1;
	compDescriptorWritesModel[2].pImageInfo = &vbid_image_info;
	compDescriptorWritesModel[2].pBufferInfo = 0;

	compDescriptorWritesModel[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[3].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[3].dstBinding = 3;
	compDescriptorWritesModel[3].dstArrayElement = 0;
	compDescriptorWritesModel[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	compDescriptorWritesModel[3].descriptorCount = 1;
	compDescriptorWritesModel[3].pImageInfo = 0;
	compDescriptorWritesModel[3].pBufferInfo = &index_buffer_info;

	compDescriptorWritesModel[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[4].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[4].dstBinding = 4;
	compDescriptorWritesModel[4].dstArrayElement = 0;
	compDescriptorWritesModel[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	compDescriptorWritesModel[4].descriptorCount = 1;
	compDescriptorWritesModel[4].pImageInfo = 0;
	compDescriptorWritesModel[4].pBufferInfo = &vertex_position_info;

	compDescriptorWritesModel[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[5].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[5].dstBinding = 5;
	compDescriptorWritesModel[5].dstArrayElement = 0;
	compDescriptorWritesModel[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	compDescriptorWritesModel[5].descriptorCount = 1;
	compDescriptorWritesModel[5].pImageInfo = 0;
	compDescriptorWritesModel[5].pBufferInfo = &start_vertex_info;

	compDescriptorWritesModel[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[6].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[6].dstBinding = 6;
	compDescriptorWritesModel[6].dstArrayElement = 0;
	compDescriptorWritesModel[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	compDescriptorWritesModel[6].descriptorCount = 1;
	compDescriptorWritesModel[6].pImageInfo = 0;
	compDescriptorWritesModel[6].pBufferInfo = &material_data_info;

	compDescriptorWritesModel[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[7].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[7].dstBinding = 7;
	compDescriptorWritesModel[7].dstArrayElement = 0;
	compDescriptorWritesModel[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	compDescriptorWritesModel[7].descriptorCount = 1;
	compDescriptorWritesModel[7].pImageInfo = 0;
	compDescriptorWritesModel[7].pBufferInfo = &directional_light_info;

	compDescriptorWritesModel[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[8].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[8].dstBinding = 8;
	compDescriptorWritesModel[8].dstArrayElement = 0;
	compDescriptorWritesModel[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	compDescriptorWritesModel[8].descriptorCount = 1;
	compDescriptorWritesModel[8].pImageInfo = &shadow_map_image_info;
	compDescriptorWritesModel[8].pBufferInfo = 0;

	compDescriptorWritesModel[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[9].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[9].dstBinding = 9;
	compDescriptorWritesModel[9].dstArrayElement = 0;
	compDescriptorWritesModel[9].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	compDescriptorWritesModel[9].descriptorCount = 1;
	compDescriptorWritesModel[9].pImageInfo = 0;
	compDescriptorWritesModel[9].pBufferInfo = &inverse_matrix_buffer_info;


	compDescriptorWritesModel[10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	compDescriptorWritesModel[10].dstSet = compositionDescriptorSet;
	compDescriptorWritesModel[10].dstBinding = 10;
	compDescriptorWritesModel[10].dstArrayElement = 0;
	compDescriptorWritesModel[10].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	compDescriptorWritesModel[10].descriptorCount = 1;
	compDescriptorWritesModel[10].pImageInfo = 0;
	compDescriptorWritesModel[10].pBufferInfo = &light_matrix_shade_buffer_info;


	vkUpdateDescriptorSets(_renderer->GetVulkanDevice(), static_cast<uint32_t>(compDescriptorWritesModel.size()), compDescriptorWritesModel.data(), 0, nullptr);






	//shadow maps
	//
	//
	//Shadow Descriptor Set alloc info
	VkDescriptorSetAllocateInfo shadowDescSetAllocInfo = {};
	shadowDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	shadowDescSetAllocInfo.descriptorPool = _descriptorPool;
	shadowDescSetAllocInfo.descriptorSetCount = 1;
	shadowDescSetAllocInfo.pSetLayouts = &shadowDescriptorSetLayout;

	vk::tools::ErrorCheck(vkAllocateDescriptorSets(_renderer->GetVulkanDevice(), &shadowDescSetAllocInfo, &shadowDescriptorSet));

	VkDescriptorBufferInfo shadow_ubo_buffer_info = {};
	shadow_ubo_buffer_info.buffer = IDMatricesUBOBuffer.buffer;
	shadow_ubo_buffer_info.offset = 0;
	shadow_ubo_buffer_info.range = sizeof(uboVS);

	VkDescriptorBufferInfo dynamic_buffer_info = {};
	dynamic_buffer_info.buffer = dynamicUboBuffer.buffer;
	dynamic_buffer_info.offset = 0;
	dynamic_buffer_info.range = sizeof(glm::mat4);

	VkDescriptorBufferInfo light_matrix_buffer_info = {};
	light_matrix_buffer_info.buffer = lightViewMatrixBuffer.buffer;
	light_matrix_buffer_info.offset = 0;
	light_matrix_buffer_info.range = sizeof(glm::mat4);



	std::vector<VkWriteDescriptorSet> shadow_descriptor_writes_model;
	//Binding 0: Vertex Shader UBO
	shadow_descriptor_writes_model.resize(3);
	shadow_descriptor_writes_model[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	shadow_descriptor_writes_model[0].dstSet = shadowDescriptorSet;
	shadow_descriptor_writes_model[0].dstBinding = 0;
	shadow_descriptor_writes_model[0].dstArrayElement = 0;
	shadow_descriptor_writes_model[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	shadow_descriptor_writes_model[0].descriptorCount = 1;
	shadow_descriptor_writes_model[0].pImageInfo = 0;
	shadow_descriptor_writes_model[0].pBufferInfo = &shadow_ubo_buffer_info;

	shadow_descriptor_writes_model[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	shadow_descriptor_writes_model[1].dstSet = shadowDescriptorSet;
	shadow_descriptor_writes_model[1].dstBinding = 1;
	shadow_descriptor_writes_model[1].dstArrayElement = 0;
	shadow_descriptor_writes_model[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	shadow_descriptor_writes_model[1].descriptorCount = 1;
	shadow_descriptor_writes_model[1].pImageInfo = 0;
	shadow_descriptor_writes_model[1].pBufferInfo = &dynamic_buffer_info;

	shadow_descriptor_writes_model[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	shadow_descriptor_writes_model[2].dstSet = shadowDescriptorSet;
	shadow_descriptor_writes_model[2].dstBinding = 2;
	shadow_descriptor_writes_model[2].dstArrayElement = 0;
	shadow_descriptor_writes_model[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	shadow_descriptor_writes_model[2].descriptorCount = 1;
	shadow_descriptor_writes_model[2].pImageInfo = 0;
	shadow_descriptor_writes_model[2].pBufferInfo = &light_matrix_buffer_info;


	vkUpdateDescriptorSets(_renderer->GetVulkanDevice(), static_cast<uint32_t>(shadow_descriptor_writes_model.size()), shadow_descriptor_writes_model.data(), 0, nullptr);





}

//Sets up the pipeline layout and descriptors for each pipeline in the renderer
void VisibilityBuffer::_CreateDescriptorSetLayout()
{
	
	//*ID Pipeline Layout and descriptors
	//====================================
	//Position layout binding (deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding ubo_layout_binding = {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = { ubo_layout_binding };
	VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
	descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();
	descriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
	descriptorLayoutCreateInfo.flags = 0;

	vk::tools::ErrorCheck(vkCreateDescriptorSetLayout(_renderer->GetVulkanDevice(), &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayouts.IDLayout));

	VkPipelineLayoutCreateInfo IDPipelineLayoutCreateInfo = {};
	IDPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	IDPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.IDLayout;
	IDPipelineLayoutCreateInfo.setLayoutCount = 1;

	vk::tools::ErrorCheck(vkCreatePipelineLayout(_renderer->GetVulkanDevice(), &IDPipelineLayoutCreateInfo, nullptr, &_pipelineLayout[PipelineType::vbID]));


	//*Vertex filtering pipeline layout and descriptors
	//=================================================
	//vk::tools::ErrorCheck(vkCreateDescriptorSetLayout(_renderer->GetVulkanDevice(), &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayouts.triangleFilterLayout));
	

	//TO:DO
	//


	//*Composition Pass pipeline laytout and descriptors
	//=================================================
	//Position layout binding (deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding ubo_shade_layout_binding = {};
	ubo_shade_layout_binding.binding = 0;
	ubo_shade_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_shade_layout_binding.descriptorCount = 1;
	ubo_shade_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Image Texture layout binding (Texture for Chalet)
	VkDescriptorSetLayoutBinding chalet_texture_layout_binding = {};
	chalet_texture_layout_binding.binding = 1;
	chalet_texture_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	chalet_texture_layout_binding.descriptorCount = static_cast<uint32_t>(_textures.size());
	chalet_texture_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Visibility Buffer texture layout binding
	VkDescriptorSetLayoutBinding ID_texture_layout_binding = {};
	ID_texture_layout_binding.binding = 2;
	ID_texture_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ID_texture_layout_binding.descriptorCount = 1;
	ID_texture_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	//Position layout binding (deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding index_buffer_layout_binding = {};
	index_buffer_layout_binding.binding = 3;
	index_buffer_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	index_buffer_layout_binding.descriptorCount = 1;
	index_buffer_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Position layout binding (deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding vertex_buffer_layout_binding = {};
	vertex_buffer_layout_binding.binding = 4;
	vertex_buffer_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertex_buffer_layout_binding.descriptorCount = 1;
	vertex_buffer_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Position layout binding (deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding start_vertex_layout_binding = {};
	start_vertex_layout_binding.binding = 5;
	start_vertex_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	start_vertex_layout_binding.descriptorCount = 1;
	start_vertex_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Position layout binding (deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding material_data_layout_binding = {};
	material_data_layout_binding.binding = 6;
	material_data_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	material_data_layout_binding.descriptorCount = 1;
	material_data_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Position layout binding (deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding directional_light_layout_binding = {};
	directional_light_layout_binding.binding = 7;
	directional_light_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	directional_light_layout_binding.descriptorCount = 1;
	directional_light_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Visibility Buffer texture layout binding
	VkDescriptorSetLayoutBinding shadow_map_layout_binding = {};
	shadow_map_layout_binding.binding = 8;
	shadow_map_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	shadow_map_layout_binding.descriptorCount = 1;
	shadow_map_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Visibility Buffer texture layout binding
	VkDescriptorSetLayoutBinding inverse_matrix_layout_binding = {};
	inverse_matrix_layout_binding.binding = 9;
	inverse_matrix_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	inverse_matrix_layout_binding.descriptorCount = 1;
	inverse_matrix_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Visibility Buffer texture layout binding
	VkDescriptorSetLayoutBinding light_matrix_layout_binding = {};
	light_matrix_layout_binding.binding = 10;
	light_matrix_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	light_matrix_layout_binding.descriptorCount = 1;
	light_matrix_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	std::vector<VkDescriptorSetLayoutBinding> compDescriptorSetLayoutBindings = { ubo_shade_layout_binding, chalet_texture_layout_binding, ID_texture_layout_binding, index_buffer_layout_binding, vertex_buffer_layout_binding, start_vertex_layout_binding, material_data_layout_binding, directional_light_layout_binding, shadow_map_layout_binding, inverse_matrix_layout_binding, light_matrix_layout_binding };
	VkDescriptorSetLayoutCreateInfo compDescriptorLayoutCreateInfo = {};
	compDescriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	compDescriptorLayoutCreateInfo.pBindings = compDescriptorSetLayoutBindings.data();
	compDescriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(compDescriptorSetLayoutBindings.size());
	compDescriptorLayoutCreateInfo.flags = 0;
	
	vk::tools::ErrorCheck(vkCreateDescriptorSetLayout(_renderer->GetVulkanDevice(), &compDescriptorLayoutCreateInfo, nullptr, &descriptorSetLayouts.compositionLayout));


	//pipeline layout push constants (For image changing)
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.size = sizeof(uint32_t);
	pushConstantRange.offset = 0;

	VkPipelineLayoutCreateInfo compPipelineLayoutCreateInfo = {};
	compPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.compositionLayout;
	compPipelineLayoutCreateInfo.setLayoutCount = 1;
	compPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	compPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	
	vk::tools::ErrorCheck(vkCreatePipelineLayout(_renderer->GetVulkanDevice(), &compPipelineLayoutCreateInfo, nullptr, &_pipelineLayout[PipelineType::vbShade]));










	//shadow layout
	//binding for the View and projection matrix
	VkDescriptorSetLayoutBinding shadow_ubo_layout_binding = {};
	shadow_ubo_layout_binding.binding = 0;
	shadow_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	shadow_ubo_layout_binding.descriptorCount = 1;
	shadow_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//Dynamic Model layout binding for the model matrices
	VkDescriptorSetLayoutBinding shadow_dynamic_model_layout_binding = {};
	shadow_dynamic_model_layout_binding.binding = 1;
	shadow_dynamic_model_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	shadow_dynamic_model_layout_binding.descriptorCount = 1;
	shadow_dynamic_model_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//binding for the View and projection matrix
	VkDescriptorSetLayoutBinding light_ubo_matrix_layout_binding = {};
	light_ubo_matrix_layout_binding.binding = 2;
	light_ubo_matrix_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	light_ubo_matrix_layout_binding.descriptorCount = 1;
	light_ubo_matrix_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


	std::vector<VkDescriptorSetLayoutBinding> shadowDescriptorSetLayoutBindings = { shadow_ubo_layout_binding, shadow_dynamic_model_layout_binding, light_ubo_matrix_layout_binding };
	VkDescriptorSetLayoutCreateInfo shadowDescriptorLayoutCreateInfo = {};
	shadowDescriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	shadowDescriptorLayoutCreateInfo.pBindings = shadowDescriptorSetLayoutBindings.data();
	shadowDescriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(shadowDescriptorSetLayoutBindings.size());
	shadowDescriptorLayoutCreateInfo.flags = 0;

	vk::tools::ErrorCheck(vkCreateDescriptorSetLayout(_renderer->GetVulkanDevice(), &shadowDescriptorLayoutCreateInfo, nullptr, &shadowDescriptorSetLayout));

	VkPipelineLayoutCreateInfo shadowPipelineLayoutCreateInfo = {};
	shadowPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	shadowPipelineLayoutCreateInfo.pSetLayouts = &shadowDescriptorSetLayout;
	shadowPipelineLayoutCreateInfo.setLayoutCount = 1;

	vk::tools::ErrorCheck(vkCreatePipelineLayout(_renderer->GetVulkanDevice(), &shadowPipelineLayoutCreateInfo, nullptr, &_pipelineLayout[PipelineType::shadowMap]));



}

//Update per frame, drawing and polling glfw windows for changes
void VisibilityBuffer::Update()
{
	//Update glfw window if the window is open
	while (!glfwWindowShouldClose(_window))
	{

		glfwOldKey = glfwNewKey;
		//poll key presses, handle camera and draw the frame to the screen
		glfwPollEvents();

		glfwNewKey = glfwGetKey(_window, GLFW_KEY_SPACE);


		// Update imGui
		ImGuiIO& io = ImGui::GetIO();
		double mousePosX, mousePosY;
		glfwGetCursorPos(_window, &mousePosX, &mousePosY);

		io.DisplaySize = ImVec2((float)_swapChainExtent.width, (float)_swapChainExtent.height);
		io.MousePos = ImVec2(mousePosX, mousePosY);
		io.MouseDown[0] = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) != 0;
		io.MouseDown[1] = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) != 0;



		VisibilityBuffer::DrawFrame();
		VisibilityBuffer::_CreateCommandBuffers();

		if (cameraUpdate)
		{
			camera->HandleInput(_window);
		}
		

		//if escape key is pressed close window
		if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(_window, GLFW_TRUE);
		}

		//if escape key is pressed close window
		if (glfwNewKey == GLFW_PRESS && glfwOldKey != GLFW_PRESS)
		{
			//Flip between true and false
			cameraUpdate == true ? cameraUpdate = false : cameraUpdate = true;
		}
		
		imGui->uiSettings.modelName = "Model: Sponza Crytek";

		imGui->UpdateImGuiInformation(cameraUpdate);

		if (imGui->uiSettings.VBIDMode == true)
		{
			drawModeValue = 1;
		}
		else
		{
			drawModeValue = 0;
		}

	}

	vkDeviceWaitIdle(_renderer->GetVulkanDevice());
}

//Renders something to the screen
void VisibilityBuffer::DrawFrame()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_renderer->GetVulkanDevice(), _swapChain, std::numeric_limits<uint64_t>::max(), presentCompleteSemaphore, VK_NULL_HANDLE, &imageIndex);
	//
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		_RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	UpdateUniformBuffer();

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.signalSemaphoreCount = 1;
	// Wait for swap chain presentation to finish
	submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
	// Signal ready with offscreen semaphore
	submitInfo.pSignalSemaphores = &IDPassSemaphore;

	// Submit work
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &IDCmdBuffer;
	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(_renderer->GetVulkanGraphicsQueue());

	vkGetQueryPoolResults(_renderer->GetVulkanDevice(), VBIDQueryPool, 0, 1, sizeof(uint32_t), &queryTimeVBIDStart, sizeof(uint32_t), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
	vkGetQueryPoolResults(_renderer->GetVulkanDevice(), VBIDQueryPool, 1, 1, sizeof(uint32_t), &queryTimeVBIDEnd, sizeof(uint32_t), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
	imGui->uiSettings.nanosecondsMRT = (queryTimeVBIDEnd - queryTimeVBIDStart);
	imGui->uiSettings.microsecondsMRT = (queryTimeVBIDEnd - queryTimeVBIDStart) / 1000;
	imGui->uiSettings.millisecondsMRT = (queryTimeVBIDEnd - queryTimeVBIDStart) / 1000000;
	frameTimeMRT = imGui->uiSettings.microsecondsMRT;



	//SHADOWS RENDERING
	submitInfo.pWaitSemaphores = &IDPassSemaphore;
	submitInfo.pSignalSemaphores = &shadowSemaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &shadowCmdBuffer;
	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));


	submitInfo.pWaitSemaphores = &shadowSemaphore;

	//// Signal ready with render complete semaphpre
	submitInfo.pSignalSemaphores = &offScreenColorSemaphore;
	//
	//// Submit work
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_drawCommandBuffers[imageIndex];

	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(_renderer->GetVulkanGraphicsQueue());



	//OFFSCREEN COLOR RENDER
	submitInfo.pWaitSemaphores = &offScreenColorSemaphore;
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &offScreenColorCmdBuffers;

	//Submit queue and time
	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(_renderer->GetVulkanGraphicsQueue());

	//Get query results from timings in the shading pass
	vkGetQueryPoolResults(_renderer->GetVulkanDevice(), VBShadeQueryPool, 0, 1, sizeof(uint32_t), &queryTimeShadingStart, sizeof(uint32_t), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
	vkGetQueryPoolResults(_renderer->GetVulkanDevice(), VBShadeQueryPool, 1, 1, sizeof(uint32_t), &queryTimeShadingEnd, sizeof(uint32_t), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
	imGui->uiSettings.nanosecondsShading = (queryTimeShadingEnd - queryTimeShadingStart);
	imGui->uiSettings.microsecondsShading = (queryTimeShadingEnd - queryTimeShadingStart)/1000;
	imGui->uiSettings.millisecondsShading = (queryTimeShadingEnd - queryTimeShadingStart)/1000000;
	frameTimeShading = imGui->uiSettings.microsecondsShading;

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &renderCompleteSemaphore;

	VkSwapchainKHR swapChains[] = { _swapChain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapChains;
	present_info.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(_renderer->GetVulkanPresentQueue(), &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _frameBufferResized)
	{
		_frameBufferResized = false;
		_RecreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	vkQueueWaitIdle(_renderer->GetVulkanPresentQueue());
}

//loads a shader file
VkPipelineShaderStageCreateInfo VisibilityBuffer::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = vk::tools::loadShader(fileName.c_str(), _renderer->GetVulkanDevice());
	shaderStage.pName = "main"; // todo : make param
	assert(shaderStage.module != VK_NULL_HANDLE);
	return shaderStage;
}




