#include "VisibilityBuffer.h"


void VisibilityBuffer::InitialiseVulkanApplication()
{
	PrepareScene();
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore));
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore));
	VisibilityBuffer::CreateImGui();
	VisibilityBuffer::CreateCamera();
	VisibilityBuffer::_CreateGeometry();
	VisibilityBuffer::CreateVBuffer();
	VisibilityBuffer::_SetUpUniformBuffers();
	VisibilityBuffer::_CreateDescriptorSetLayout();
	VisibilityBuffer::_CreateVertexDescriptions();
	VisibilityBuffer::_CreateGraphicsPipeline();
	VisibilityBuffer::_CreateDescriptorPool();
	VisibilityBuffer::_CreateDescriptorSets();
	VisibilityBuffer::_CreateCommandBuffers();
	VisibilityBuffer::_CreateVIDCommandBuffers();
	VisibilityBuffer::GiveImGuiStaticInformation();
	VisibilityBuffer::Update();
}


void VisibilityBuffer::GiveImGuiStaticInformation()
{
	for (int i = 0; i < _models.size(); i++)
	{
		imGui->uiSettings.indices += _models[i]->model->GetIndexCount();
		imGui->uiSettings.vertices += _models[i]->model->GetVertexCount();
		imGui->uiSettings.face += (_models[i]->model->GetVertexCount() / 3);
	}
}

void VisibilityBuffer::CreateImGui()
{
	imGui = new ImGUIInterface(_renderer);
	imGui->init((float)_swapChainExtent.width, (float)_swapChainExtent.height);
	imGui->initResources(_renderPass, _renderer->GetVulkanGraphicsQueue());
}

void VisibilityBuffer::CreateCamera()
{
	camera = new Camera(glm::vec3(-2.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 60.0f, glm::vec2(_swapChainExtent.width, _swapChainExtent.height), 0.1f, 200.0f);
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

	houseModel = new ImportedModel("Textures/chalet.obj", true);
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), houseModel->GetVertexBufferSize(), &houseModel->GetVertexBuffer()->buffer, &houseModel->GetVertexBuffer()->memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, houseModel->GetVertexData());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), houseModel->GetIndexBufferSize(), &houseModel->GetIndexBuffer()->buffer, &houseModel->GetIndexBuffer()->memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, houseModel->GetIndexData());
	houseModel->GetIndexBuffer()->SetUpDescriptorSet();
	houseModel->GetVertexBuffer()->SetUpDescriptorSet();


	//Loads the sponza scene
	std::vector<std::string> textureFilepaths;
	
	//Crytek Sponza scene
	BaseModel::LoadMeshFromFile("Textures/Sponza/sponza.obj", "Textures/Sponza/", true, _meshes, textureFilepaths);
	
	//San_Miguel scene
	//BaseModel::LoadMeshFromFile("Textures/San_Miguel/san-miguel-low-poly.obj", "Textures/San_Miguel/san-miguel-low-poly.mtl", true, _meshes, textureFilepaths);
	
	for (int i = 0; i < textureFilepaths.size(); i++)
	{
		_textures.push_back(vk::wrappers::Texture2D());
		_textures[i].image = _CreateTextureImage(textureFilepaths[i].c_str());
		_textures[i].imageView = _CreateTextureImageView(_textures[i].image);
		_textures[i].sampler = _CreateTextureSampler();
	}
	
	
	for (int i = 0; i < _meshes.size(); i++)
	{
		sceneVertices.reserve(sceneVertices.size() + _meshes[i]->vertices.size());
		sceneIndices.reserve(sceneIndices.size() + _meshes[i]->indices.size());
		sceneVertices.insert(sceneVertices.begin(), _meshes[i]->vertices.begin(), _meshes[i]->vertices.end());
		sceneIndices.insert(sceneIndices.begin(), _meshes[i]->indices.begin(), _meshes[i]->indices.end());


		_CreateShaderBuffer(_renderer->GetVulkanDevice(), _meshes[i]->GetVertexBufferSize(), &_meshes[i]->GetVertexBuffer()->buffer, &_meshes[i]->GetVertexBuffer()->memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, _meshes[i]->GetVertexData());
		_CreateShaderBuffer(_renderer->GetVulkanDevice(), _meshes[i]->GetIndexBufferSize(), &_meshes[i]->GetIndexBuffer()->buffer, &_meshes[i]->GetIndexBuffer()->memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, _meshes[i]->GetIndexData());
		_meshes[i]->GetIndexBuffer()->SetUpDescriptorSet();
		_meshes[i]->GetVertexBuffer()->SetUpDescriptorSet();
	
		_models.push_back(new vk::wrappers::Model());
		_models[i]->model = _meshes[i];
	}


	//Vertex and index buffer for the shading pass of the visibility buffer
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), sceneVertices.size() * sizeof(Vertex), &sceneVertexBuffer.buffer, &sceneVertexBuffer.memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sceneVertices.data());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), sceneIndices.size() * sizeof(uint32_t), &sceneIndexBuffer.buffer, &sceneIndexBuffer.memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sceneIndices.data());




	//*Lighting*
	_spotLights.push_back(new vk::wrappers::SpotLight());
	_pointLights.push_back(new vk::wrappers::PointLight());
	_directionalLights.push_back(new vk::wrappers::DirectionalLight());

	_directionalLights[0]->diffuse = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
	_directionalLights[0]->direction = glm::vec4(1.0f, 2.0f, 0.0f, 1.0f);
	_directionalLights[0]->specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	testLightViewMatrix = glm::perspective(glm::radians(100.0f), 1.0f, 1.0f, 64.0f);
	//ortho(10.0f, -10.0f, -10.0f, 10.0f, 1.0f, 10.0f);
	glm::mat4 lightView = glm::lookAt(glm::vec3(2.0f, 4.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 shadowModel = glm::mat4(1.0f);
	glm::mat4 lightSpaceMatrix = testLightViewMatrix * lightView * shadowModel;
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), sizeof(glm::mat4), &lightViewMatrixBuffer.buffer, &lightViewMatrixBuffer.memory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &lightSpaceMatrix);

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


	//Final Pipeline (after offscreen pass)
	//Shader loading (Loads shader modules for pipeline)
	shaderStages[0] = loadShader("Shaders/VisibilityBuffer/VBShade.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/VisibilityBuffer/VBShade.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineVertexInputStateCreateInfo emptyVertexInputState = {};
	emptyVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;

	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::vbShade];

	//Final composition graphics pipeline
	vk::tools::ErrorCheck(vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipelines[PipelineType::vbShade]));

	//Visibility buffer ID graphics pipeline
	//=========================
	shaderStages[0] = loadShader("Shaders/VisibilityBuffer/VBID.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/VisibilityBuffer/VBID.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.pVertexInputState = &vertices.inputState;

	//rasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
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
			*modelMat = glm::translate(*modelMat, glm::vec3(0.0f, -1.0f, 0.0f));
			//*modelMat = glm::rotate(*modelMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			*modelMat = glm::scale(*modelMat, glm::vec3(0.05f, 0.05f, 0.05f));
		}
		else if (i == 1)
		{
			*modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			*modelMat = glm::rotate(*modelMat, glm::radians(-180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			*modelMat = glm::scale(*modelMat, glm::vec3(4.0f, 4.0f, 4.0f));
		}
	}

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

	void* data;
	vkMapMemory(_renderer->GetVulkanDevice(), IDMatricesUBOBuffer.memory, 0, sizeof(uboVS), 0, &data);
	memcpy(data, &IDMatricesVSData, sizeof(uboVS));
	IDMatricesUBOBuffer.SetUpDescriptorSet();
}

//Updates the uniform buffers for the descriptor sets
void VisibilityBuffer::UpdateUniformBuffer()
{
	IDMatricesVSData.model = glm::mat4(1.0f);
	IDMatricesVSData.model = glm::scale(IDMatricesVSData.model, glm::vec3(0.005f, 0.005f, 0.005f));
	IDMatricesVSData.view = camera->GetViewMatrix();
	IDMatricesVSData.projection = camera->GetProjectionMatrix();

	void* data;
	vkMapMemory(_renderer->GetVulkanDevice(), IDMatricesUBOBuffer.memory, 0, sizeof(uboVS), 0, &data);
	memcpy(data, &IDMatricesVSData, sizeof(uboVS));
	vkUnmapMemory(_renderer->GetVulkanDevice(), IDMatricesUBOBuffer.memory);
	IDMatricesUBOBuffer.SetUpDescriptorSet();
}

//Creates visibility buffer attachments for the first render pass 
void VisibilityBuffer::CreateVBuffer()
{
	IDFrameBuffer.width = _swapChainExtent.width;
	IDFrameBuffer.height = _swapChainExtent.height;

	_CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &IDFrameBuffer.VID, IDFrameBuffer);
	
	//Find Depth Format
	VkFormat DepthFormat;
	DepthFormat = _FindDepthFormat();

	_CreateAttachment(DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &IDFrameBuffer.depth, IDFrameBuffer);

	//Attachment descriptions for renderpass 
	std::array<VkAttachmentDescription, 2> attachmentDescs = {};
	for (uint32_t i = 0; i < 2; ++i)
	{
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
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


		for (int i = 0; i < _models.size(); i++)
		{
			vkCmdBindPipeline(IDCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::vbID]);
			vkCmdBindDescriptorSets(IDCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::vbID], 0, 1, &descriptorSets.house, 0, NULL);
			vkCmdBindVertexBuffers(IDCmdBuffer, 0, 1, &_models[i]->model->GetVertexBuffer()->buffer, offsets);
			vkCmdBindIndexBuffer(IDCmdBuffer, _models[i]->model->GetIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(IDCmdBuffer, _models[i]->model->GetIndexCount(), 1, 0, 0, 0);
		}
		
			//House
	

	vkCmdEndRenderPass(IDCmdBuffer);

	//End the command buffer drawing
	vk::tools::ErrorCheck(vkEndCommandBuffer(IDCmdBuffer));

}

//Second pass for the visibility buffer: Filters triangles using culling methods to reduce triangle batches
//*Second pass for Visibility buffer
void VisibilityBuffer::_CreateVertexFilteringCommandBuffers()
{
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
		diffuse_array_image_info[i].sampler = colorSampler;
		diffuse_array_image_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		diffuse_array_image_info[i].imageView = _textures[i].imageView;
	}

	VkDescriptorImageInfo vbid_image_info = {};
	vbid_image_info.sampler = colorSampler;
	vbid_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vbid_image_info.imageView = IDFrameBuffer.VID.view;

	VkDescriptorBufferInfo index_buffer_info = {};
	index_buffer_info.buffer = sceneIndexBuffer.buffer;
	index_buffer_info.offset = 0;
	index_buffer_info.range = static_cast<uint32_t>(sceneIndices.size()) * sizeof(uint32_t);

	VkDescriptorBufferInfo vertex_position_info = {};
	vertex_position_info.buffer = sceneVertexBuffer.buffer;
	vertex_position_info.offset = 0;
	vertex_position_info.range = static_cast<uint32_t>(sceneVertices.size()) * sizeof(Vertex);

	std::vector<VkWriteDescriptorSet> compDescriptorWritesModel;
	//Binding 0: Vertex Shader UBO
	compDescriptorWritesModel.resize(5);
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



	vkUpdateDescriptorSets(_renderer->GetVulkanDevice(), static_cast<uint32_t>(compDescriptorWritesModel.size()), compDescriptorWritesModel.data(), 0, nullptr);

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


	std::vector<VkDescriptorSetLayoutBinding> compDescriptorSetLayoutBindings = { ubo_shade_layout_binding, chalet_texture_layout_binding, ID_texture_layout_binding, index_buffer_layout_binding, vertex_buffer_layout_binding };
	VkDescriptorSetLayoutCreateInfo compDescriptorLayoutCreateInfo = {};
	compDescriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	compDescriptorLayoutCreateInfo.pBindings = compDescriptorSetLayoutBindings.data();
	compDescriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(compDescriptorSetLayoutBindings.size());
	compDescriptorLayoutCreateInfo.flags = 0;
	
	vk::tools::ErrorCheck(vkCreateDescriptorSetLayout(_renderer->GetVulkanDevice(), &compDescriptorLayoutCreateInfo, nullptr, &descriptorSetLayouts.compositionLayout));

	VkPipelineLayoutCreateInfo compPipelineLayoutCreateInfo = {};
	compPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.compositionLayout;
	compPipelineLayoutCreateInfo.setLayoutCount = 1;
	
	vk::tools::ErrorCheck(vkCreatePipelineLayout(_renderer->GetVulkanDevice(), &compPipelineLayoutCreateInfo, nullptr, &_pipelineLayout[PipelineType::vbShade]));
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

		imGui->uiSettings.modelName = "Model: Penguin";

		imGui->UpdateImGuiInformation(cameraUpdate);
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
	auto start = std::chrono::high_resolution_clock::now();
	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(_renderer->GetVulkanGraphicsQueue());
	auto end = std::chrono::high_resolution_clock::now();
	frameTimeMRT = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	//// Scene rendering
	//// Wait for offscreen semaphore
	submitInfo.pWaitSemaphores = &IDPassSemaphore;
	//// Signal ready with render complete semaphpre
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;
	//
	//// Submit work
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_drawCommandBuffers[imageIndex];

	start = std::chrono::high_resolution_clock::now();
	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(_renderer->GetVulkanGraphicsQueue());
	end = std::chrono::high_resolution_clock::now();
	frameTimeShading = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();


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




