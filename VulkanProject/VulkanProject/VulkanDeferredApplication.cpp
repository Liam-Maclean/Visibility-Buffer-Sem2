#include "VulkanDeferredApplication.h"

void VulkanDeferredApplication::InitialiseVulkanApplication()
{
	PrepareScene();
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore));
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore));
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &shadowSemaphore));
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &offScreenColorSemaphore));
	VulkanDeferredApplication::CreateImGui();
	VulkanDeferredApplication::CreateCamera();
	VulkanDeferredApplication::_CreateGeometry();
	VulkanDeferredApplication::CreateQueryPools();
	VulkanDeferredApplication::CreateShadowRenderPass();
	VulkanDeferredApplication::_CreateOffScreenColorRenderPass();
	VulkanDeferredApplication::CreateGBuffer();
	VulkanDeferredApplication::CreateIndirectInformation();
	VulkanDeferredApplication::SetUpUniformBuffers();
	VulkanDeferredApplication::_CreateDescriptorSetLayout();
	VulkanDeferredApplication::_CreateVertexDescriptions();
	VulkanDeferredApplication::_CreateGraphicsPipeline();
	VulkanDeferredApplication::_CreateOffScreenColorPipeline();
	VulkanDeferredApplication::_CreateDescriptorPool();
	VulkanDeferredApplication::_CreateDescriptorSets();
	VulkanDeferredApplication::CreateDeferredCommandBuffers();
	VulkanDeferredApplication::_CreateCommandBuffers();
	VulkanDeferredApplication::CreateShadowPassCommandBuffers();
	VulkanDeferredApplication::_CreateOffScreenColorCommandBuffers();
	VulkanDeferredApplication::GiveImGuiStaticInformation();
	VulkanDeferredApplication::Update();
}

VulkanDeferredApplication::~VulkanDeferredApplication()
{
	vkDestroyDescriptorSetLayout(_renderer->GetVulkanDevice(), offScreenDescriptorSetLayout, nullptr);
	vkDestroySampler(_renderer->GetVulkanDevice(), colorSampler, nullptr);
	vkDestroySemaphore(_renderer->GetVulkanDevice(), presentCompleteSemaphore, nullptr);
	vkDestroySemaphore(_renderer->GetVulkanDevice(), renderCompleteSemaphore, nullptr);
	vkDestroySemaphore(_renderer->GetVulkanDevice(), offScreenSemaphore, nullptr);
	vkDestroySemaphore(_renderer->GetVulkanDevice(), shadowSemaphore, nullptr);
}

//Update
void VulkanDeferredApplication::Update()
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

		VulkanDeferredApplication::DrawFrame();
		VulkanDeferredApplication::_CreateCommandBuffers();

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

		imGui->uiSettings.modelName = "Model: Sphere";

		imGui->UpdateImGuiInformation(cameraUpdate);




		if (imGui->uiSettings.positionMode == true)
		{
			drawModeValue = 1;
		}
		else if (imGui->uiSettings.normalMode == true)
		{
			drawModeValue = 2;
		}
		else if (imGui->uiSettings.albedoMode == true)
		{
			drawModeValue = 3;
		}
		else
		{
			drawModeValue = 0;
		}


	}
	vkDeviceWaitIdle(_renderer->GetVulkanDevice());
}

//Creates indirect information for VkCmdIndirectIndexedDraw
void VulkanDeferredApplication::CreateIndirectInformation()
{
	int32_t m = 0;

	materialIDs.resize(_models[0]->model->parts.size());


	for (size_t i = 0; i < _models[0]->model->parts.size(); i++)
	{
		VkDrawIndexedIndirectCommand drawIndexedCmd = {};
		drawIndexedCmd.instanceCount = 1;
		drawIndexedCmd.firstInstance = 0;
		drawIndexedCmd.firstIndex = _models[0]->model->parts[i].indexBase;
		drawIndexedCmd.indexCount = _models[0]->model->parts[i].indexCount;

		materialIDs[i].materialID = _models[0]->model->parts[i].materialID;

		indirectCommands.push_back(drawIndexedCmd);
		m++;
	}

	indirectDrawCount = indirectDrawCount = static_cast<uint32_t>(indirectCommands.size());


	_CreateShaderBuffer(_renderer->GetVulkanDevice(), indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand), &indirectCommandsBuffer.buffer, &indirectCommandsBuffer.memory, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, indirectCommands.data());
}

//Creates the camera in which the view and projection matrices are held
void VulkanDeferredApplication::CreateCamera()
{
	//Creates the camera with camera values to create view and projection matrices 
	cameraEye = glm::vec4(-2.0f, 2.0f, 3.0f, 1.0f);
	camera = new Camera(glm::vec3(cameraEye.x, cameraEye.y, cameraEye.z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 60.0f, glm::vec2(_swapChainExtent.width, _swapChainExtent.height), 0.1f, 200.0f);
	
}

//Creates query pools for timestamps in the framework later on
void VulkanDeferredApplication::CreateQueryPools()
{
	VkQueryPoolCreateInfo VBIDQueryInfo = {};
	VBIDQueryInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	VBIDQueryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	VBIDQueryInfo.pNext = 0;
	VBIDQueryInfo.flags = 0;
	VBIDQueryInfo.queryCount = 2;

	vkCreateQueryPool(_renderer->GetVulkanDevice(), &VBIDQueryInfo, nullptr, &DeferredMRTQueryPool);


	VkQueryPoolCreateInfo VBShadeQueryInfo = {};
	VBShadeQueryInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	VBShadeQueryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	VBShadeQueryInfo.pNext = 0;
	VBShadeQueryInfo.flags = 0;
	VBShadeQueryInfo.queryCount = 2;

	vkCreateQueryPool(_renderer->GetVulkanDevice(), &VBShadeQueryInfo, nullptr, &DeferredShadeQueryPool);
}

//Method to give ImGui the static information on the UI that wont change much (Vertex data, indices data, face data, model name)
void VulkanDeferredApplication::GiveImGuiStaticInformation()
{
	for (int i = 0; i < _models.size(); i++)
	{
		imGui->uiSettings.indices += _models[i]->model->GetIndexCount();
		imGui->uiSettings.vertices += _models[i]->model->GetVertexCount();
		imGui->uiSettings.face += (_models[i]->model->GetIndexCount() / 3);
	}


}

//Draw frame
void VulkanDeferredApplication::DrawFrame()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_renderer->GetVulkanDevice(), _swapChain, std::numeric_limits<uint64_t>::max(),presentCompleteSemaphore, VK_NULL_HANDLE, &imageIndex);
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

	UpdateUniformBuffer(imageIndex);



	vkDeviceWaitIdle(_renderer->GetVulkanDevice());
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.signalSemaphoreCount = 1;
	// Wait for swap chain presentation to finish and ready submit info to fill G-Buffer
	submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
	submitInfo.pSignalSemaphores = &shadowSemaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &offScreenCmdBuffer;


	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(_renderer->GetVulkanGraphicsQueue());
	vkGetQueryPoolResults(_renderer->GetVulkanDevice(), DeferredMRTQueryPool, 0, 1, sizeof(uint32_t)*2, &queryTimeMRTStart, sizeof(uint32_t), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
	vkGetQueryPoolResults(_renderer->GetVulkanDevice(), DeferredMRTQueryPool, 1, 1, sizeof(uint32_t)*2, &queryTimeMRTEnd, sizeof(uint32_t), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
	imGui->uiSettings.nanosecondsMRT = (queryTimeMRTEnd - queryTimeMRTStart);
	imGui->uiSettings.microsecondsMRT = (queryTimeMRTEnd - queryTimeMRTStart) / 1000;
	imGui->uiSettings.millisecondsMRT = (queryTimeMRTEnd - queryTimeMRTStart) / 1000000;
	frameTimeMRT = imGui->uiSettings.microsecondsMRT;



	//Set up submit info for shadow pass
	submitInfo.pWaitSemaphores = &shadowSemaphore;
	submitInfo.pSignalSemaphores = &offScreenSemaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &shadowCmdBuffer;
	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(_renderer->GetVulkanGraphicsQueue());



	//Set up submit info for framebuffer pass
	submitInfo.pWaitSemaphores = &offScreenSemaphore;
	submitInfo.pSignalSemaphores = &offScreenColorSemaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_drawCommandBuffers[imageIndex];

	//Time the queue duration
	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(_renderer->GetVulkanGraphicsQueue());


	//Set up submit info for framebuffer pass
	submitInfo.pWaitSemaphores = &offScreenColorSemaphore;
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &offScreenColorCmdBuffers;

	//Time the queue duration
	vk::tools::ErrorCheck(vkQueueSubmit(_renderer->GetVulkanGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(_renderer->GetVulkanGraphicsQueue());

	vkGetQueryPoolResults(_renderer->GetVulkanDevice(), DeferredShadeQueryPool, 0, 1, sizeof(uint32_t)*2, &queryTimeShadeStart, sizeof(uint32_t), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
	vkGetQueryPoolResults(_renderer->GetVulkanDevice(), DeferredShadeQueryPool, 1, 1, sizeof(uint32_t)*2, &queryTimeShadeEnd, sizeof(uint32_t), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
	imGui->uiSettings.nanosecondsShading = (queryTimeShadeEnd - queryTimeShadeStart);
	imGui->uiSettings.microsecondsShading = (queryTimeShadeEnd - queryTimeShadeStart) / 1000;
	imGui->uiSettings.millisecondsShading = (queryTimeShadeEnd - queryTimeShadeStart) / 1000000;
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

//Creates ImGui information for UI
void VulkanDeferredApplication::CreateImGui()
{
	imGui = new ImGUIInterface(_renderer);
	imGui->init((float)_swapChainExtent.width, (float)_swapChainExtent.height, "Vulkan Deferred Rendering", ImGuiMode::DeferredMode);
	imGui->initResources(_renderPass, _renderer->GetVulkanGraphicsQueue());
}

//Creates the graphics pipelines for the deferred renderer (offscreen and fullscreen pipelines)
void VulkanDeferredApplication::_CreateGraphicsPipeline()
{
	//base forward rendering pipeline creation
	//VulkanWindow::_CreateGraphicsPipeline();

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
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::deferred];
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
	shaderStages[0] = loadShader("Shaders/Deferred/deferred.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/Deferred/deferred.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	shaderStages[1].pSpecializationInfo = &specializationInfo;

	VkPipelineVertexInputStateCreateInfo emptyVertexInputState = {};
	emptyVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	multisampleStateInfo.rasterizationSamples = sampleCount;
	pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
	
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::deferred];

	vk::tools::ErrorCheck(vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipelines[PipelineType::deferred]));
	pipelineCreateInfo.pVertexInputState = &vertices.inputState;


	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;


	//Offscreen pass pipeline (before drawing)
	shaderStages[0] = loadShader("Shaders/Deferred/mrt.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/Deferred/mrt.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	//

	multisampleStateInfo.rasterizationSamples = sampleCount;
	multisampleStateInfo.alphaToCoverageEnable = VK_TRUE;

	////Swap the render pass and layout to the deferred renderpass and layout (offscreen rendering)
	pipelineCreateInfo.renderPass = deferredOffScreenFrameBuffer.renderPass;
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::offscreen];
	//
	std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {};


	VkPipelineColorBlendAttachmentState attachmentState1 = {};
	attachmentState1.colorWriteMask = 0xf;
	attachmentState1.blendEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState attachmentState2 = {};
	attachmentState2.colorWriteMask = 0xf;
	attachmentState2.blendEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState attachmentState3 = {};
	attachmentState3.colorWriteMask = 0xf;
	attachmentState3.blendEnable = VK_FALSE;

	blendAttachmentStates = { attachmentState1, attachmentState2, attachmentState3 };
	
	////Blend attachments for the pipeline
	blendStateInfo.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
	blendStateInfo.pAttachments = blendAttachmentStates.data();

	vk::tools::ErrorCheck(vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipelines[PipelineType::offscreen]));

	_CreateShadowPipeline();
	//vk::tools::ReadShaderFile()

}

//Creates the pipeline for the shadow pass of the renderer
void VulkanDeferredApplication::_CreateShadowPipeline()
{
	//base forward rendering pipeline creation
	//VulkanWindow::_CreateGraphicsPipeline();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
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

	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineVertexInputStateCreateInfo emptyVertexInputState = {};
	emptyVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	pipelineCreateInfo.pVertexInputState = &vertices.inputState;

	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::shadowMap];

	vk::tools::ErrorCheck(vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipelines[PipelineType::shadowMap]));
}

//Updates the uniform buffers
void VulkanDeferredApplication::UpdateUniformBuffer(uint32_t currentImage)
{
	offScreenUniformVSData.view = camera->GetViewMatrix();
	offScreenUniformVSData.projection = camera->GetProjectionMatrix();
	
	
	void* data;
	vkMapMemory(_renderer->GetVulkanDevice(), offScreenVertexUBOBuffer.memory, 0, sizeof(uboVS), 0, &data);
	memcpy(data, &offScreenUniformVSData, sizeof(uboVS));
	vkUnmapMemory(_renderer->GetVulkanDevice(), offScreenVertexUBOBuffer.memory);
	offScreenVertexUBOBuffer.SetUpDescriptorSet();
	
	
	void* data2;
	vkMapMemory(_renderer->GetVulkanDevice(), cameraEyeBuffer.memory, 0, sizeof(glm::vec4), 0, &data2);
	memcpy(data2, &camera->GetCameraEye(), sizeof(glm::vec4));
	vkUnmapMemory(_renderer->GetVulkanDevice(), cameraEyeBuffer.memory);
	cameraEyeBuffer.SetUpDescriptorSet();
}

//loads a shader file
VkPipelineShaderStageCreateInfo VulkanDeferredApplication::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = vk::tools::loadShader(fileName.c_str(), _renderer->GetVulkanDevice());
	shaderStage.pName = "main"; // todo : make param
	assert(shaderStage.module != VK_NULL_HANDLE);
	return shaderStage;
}

//Generates the quads to display the final result of the combined full screen buffers
void VulkanDeferredApplication::_CreateGeometry()
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

	houseModel = new ImportedModel("Textures/salle_de_bain/salle_de_bain.obj", true, "Textures/salle_de_bain/", textureFilepaths);
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
	//}

	




	//Models
	//_models.push_back(new vk::wrappers::Model());
	//_models.push_back(new vk::wrappers::Model());
	//_models[0]->model = houseModel;
	//_models[0]->texture.image = _CreateTextureImage("Textures/DefaultTexture.jpg");
	//_models[0]->texture.imageView = _CreateTextureImageView(_models[0]->texture.image);
	//_models[0]->texture.sampler = _CreateTextureSampler();
	//_models[1]->model = planeMesh;
	//_models[1]->texture.image = _CreateTextureImage("Textures/BrickTexture.jpg");
	//_models[1]->texture.imageView = _CreateTextureImageView(_models[1]->texture.image);
	//_models[1]->texture.sampler = _CreateTextureSampler();

	//*Lighting*
	_spotLights.push_back(new vk::wrappers::SpotLight());
	_pointLights.push_back(new vk::wrappers::PointLight());
	_directionalLights.push_back(new vk::wrappers::DirectionalLight());
	_directionalLights.push_back(new vk::wrappers::DirectionalLight());

	_directionalLights[0]->diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	_directionalLights[0]->direction = glm::vec4(2.0f, 7.0f, 2.0f, 1.0f);
	_directionalLights[0]->specular = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

	_pointLights[0]->diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	_pointLights[0]->position = glm::vec4(1.0f, 3.5f, 1.0f,1.0f);
	_pointLights[0]->specular = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	_pointLights[0]->constant = 1.0f;
	_pointLights[0]->linear = 0.09f;
	_pointLights[0]->quadratic = 0.032f;

	//Create lights storage buffers to pass to the fragment shader
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), _spotLights.size() * sizeof(vk::wrappers::SpotLight), &_spotLightBuffer, &_spotLightBufferMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, *_spotLights.data());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), _pointLights.size() * sizeof(vk::wrappers::PointLight), &_pointLightBuffer, &_pointLightBufferMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, *_pointLights.data());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), _directionalLights.size() * sizeof(vk::wrappers::DirectionalLight), &_directionalLightBuffer, &_directionalLightBufferMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, *_directionalLights.data());
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), sizeof(glm::vec4), &cameraEyeBuffer.buffer, &cameraEyeBuffer.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &camera->GetCameraEye());
	cameraEyeBuffer.SetUpDescriptorSet();



	//Shadow matrices
	glm::mat4 testLightViewMatrix = glm::mat4(1.0f);
	testLightViewMatrix = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, -30.0f, 30.0f);
	glm::mat4 perspectView = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 60.0f);
	lightView = glm::mat4(1.0f);
	lightView = glm::lookAt(glm::vec3(2.0f, 7.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	finalLightMatrix = testLightViewMatrix * lightView * glm::mat4(1.0f);
	inverseFinalLightMatrix =  finalLightMatrix;
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), sizeof(glm::mat4), &lightViewMatrixBuffer.buffer, &lightViewMatrixBuffer.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &finalLightMatrix);
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), sizeof(glm::mat4), &lightViewMatrixBuffershading.buffer, &lightViewMatrixBuffershading.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &inverseFinalLightMatrix);


}

//Creates a render pass
void VulkanDeferredApplication::_CreateRenderPass()
{
	//Base forward rendering render pass creation
	VulkanWindow::_CreateRenderPass();
}

//Method for creating a memory buffer for shaders
//*Performs the staging buffer process to move to GPU memory
void VulkanDeferredApplication::_CreateShaderBuffer(VkDevice device, VkDeviceSize size, VkBuffer * buffer, VkDeviceMemory* memory, VkBufferUsageFlagBits bufferStage, void* data)
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

//Sets up uniform buffers to be passed to the shaders
void VulkanDeferredApplication::SetUpUniformBuffers()
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


		
		*modelMat = glm::mat4(1.0f);
		*modelMat = glm::translate(*modelMat, glm::vec3(0.0f, -2.0f, 0.0f));
		//*modelMat = glm::rotate(*modelMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		*modelMat = glm::scale(*modelMat, glm::vec3(0.1, 0.1, 0.1));
		
		//else if (i == 0)
		//{ 
		//	*modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		//	*modelMat = glm::rotate(*modelMat, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//	*modelMat = glm::scale(*modelMat, glm::vec3(8.0f, 8.0f, 8.0f));
		//}
	}
	

	VulkanDeferredApplication::_CreateShaderBuffer(_renderer->GetVulkanDevice(), static_cast<uint32_t>(materialIDs.size()) * sizeof(materialInfo), &materialIDBuffer.buffer, &materialIDBuffer.memory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, materialIDs.data());

	//Creates dynamic uniform buffer
	//_CreateShaderBuffer(_renderer->GetVulkanDevice(), bufferSize, &dynamicUboBuffer.buffer, &dynamicUboBuffer.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &uboDataDynamic);
	//Maps the data to the dynamic uniform buffer
	void* data2;
	vkMapMemory(_renderer->GetVulkanDevice(), dynamicUboBuffer.memory, 0, bufferSize, 0, &data2);
	memcpy(data2, uboDataDynamic.model, bufferSize);
	dynamicUboBuffer.SetUpDescriptorSet();

	//Vertex UBO
	_CreateShaderBuffer(_renderer->GetVulkanDevice(), sizeof(uboVS), &offScreenVertexUBOBuffer.buffer, &offScreenVertexUBOBuffer.memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &offScreenUniformVSData);
	offScreenUniformVSData.view = glm::lookAt(glm::vec3(2.0f, -2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	offScreenUniformVSData.projection = glm::perspective(glm::radians(45.0f), _swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 10.0f);

	void* data = nullptr;
	vkMapMemory(_renderer->GetVulkanDevice(), offScreenVertexUBOBuffer.memory, 0, sizeof(uboVS), 0, &data);
	memcpy(data, &offScreenUniformVSData, sizeof(uboVS));
	offScreenVertexUBOBuffer.SetUpDescriptorSet();
}

//Creates the shadow render pass
void VulkanDeferredApplication::CreateShadowRenderPass()
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

//Creates the offscreen framebuffer and attachments for the G-Buffer
void VulkanDeferredApplication::CreateGBuffer()
{
	//Off Screen framebuffer for deferred rendering

	//deferredOffScreenFrameBuffer.width = _swapChainExtent.width;
	//deferredOffScreenFrameBuffer.height = _swapChainExtent.height;


	deferredOffScreenFrameBuffer.width = OFFSCREEN_WIDTH;
	deferredOffScreenFrameBuffer.height = OFFSCREEN_HEIGHT;

	//Create the color attachments for each screen render
	_CreateAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &deferredOffScreenFrameBuffer.position, deferredOffScreenFrameBuffer);
	_CreateAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &deferredOffScreenFrameBuffer.normal, deferredOffScreenFrameBuffer);
	_CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &deferredOffScreenFrameBuffer.albedo, deferredOffScreenFrameBuffer);

	//depth attachment

	//Find Depth Format
	VkFormat DepthFormat;
	DepthFormat = _FindDepthFormat();

	_CreateAttachment(DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &deferredOffScreenFrameBuffer.depth, deferredOffScreenFrameBuffer);
	

	//Attachment descriptions for renderpass 
	std::array<VkAttachmentDescription, 4> attachmentDescs = {};
	for (uint32_t i = 0; i < 4; ++i)
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
	attachmentDescs[0].format = deferredOffScreenFrameBuffer.position.format;
	attachmentDescs[1].format = deferredOffScreenFrameBuffer.normal.format;
	attachmentDescs[2].format = deferredOffScreenFrameBuffer.albedo.format;
	attachmentDescs[3].format = deferredOffScreenFrameBuffer.depth.format;

	std::vector<VkAttachmentReference> colorReferences;
	colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 3;
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
	vk::tools::ErrorCheck(vkCreateRenderPass(_renderer->GetVulkanDevice(), &renderPassInfo, nullptr, &deferredOffScreenFrameBuffer.renderPass));

	std::array<VkImageView, 4> attachments;
	attachments[0] = deferredOffScreenFrameBuffer.position.view;
	attachments[1] = deferredOffScreenFrameBuffer.normal.view;
	attachments[2] = deferredOffScreenFrameBuffer.albedo.view;
	attachments[3] = deferredOffScreenFrameBuffer.depth.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = deferredOffScreenFrameBuffer.renderPass;
	frameBufferCreateInfo.pAttachments = attachments.data();
	frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	frameBufferCreateInfo.width = deferredOffScreenFrameBuffer.width;
	frameBufferCreateInfo.height = deferredOffScreenFrameBuffer.height;
	frameBufferCreateInfo.layers = 1;
	vk::tools::ErrorCheck(vkCreateFramebuffer(_renderer->GetVulkanDevice(), &frameBufferCreateInfo, nullptr, &deferredOffScreenFrameBuffer.frameBuffer));


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
	vk::tools::ErrorCheck(vkCreateSampler(_renderer->GetVulkanDevice(), &samplerCreateInfo, nullptr, &colorSampler));


}

//Creates the descriptor write and read sets for buffers
void VulkanDeferredApplication::_CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(_swapChainImages.size(), deferredDescriptorSetLayout);

	//For rendering the quad
	VkDescriptorSetAllocateInfo descSetAllocInfo {};
	descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo.descriptorPool = _descriptorPool;
	descSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(_swapChainImages.size());
	descSetAllocInfo.pSetLayouts = layouts.data();

	_descriptorSets.resize(_swapChainImages.size());
	vk::tools::ErrorCheck(vkAllocateDescriptorSets(_renderer->GetVulkanDevice(), &descSetAllocInfo, _descriptorSets.data()));
	for (size_t i = 0; i < _swapChainImages.size(); i++)
	{
		
		//Image descriptors for offscreen color attachments
		VkDescriptorImageInfo texDescriptorPosition = {};
		texDescriptorPosition.sampler = colorSampler;
		texDescriptorPosition.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texDescriptorPosition.imageView = deferredOffScreenFrameBuffer.position.view;

		VkDescriptorImageInfo texDescriptorNormal = {};
		texDescriptorNormal.sampler = colorSampler;
		texDescriptorNormal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texDescriptorNormal.imageView = deferredOffScreenFrameBuffer.normal.view;

		VkDescriptorImageInfo texDescriptorAlbedo = {};
		texDescriptorAlbedo.sampler = colorSampler;
		texDescriptorAlbedo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texDescriptorAlbedo.imageView = deferredOffScreenFrameBuffer.albedo.view;

		//spot lights
		VkDescriptorBufferInfo spotLightDescriptorInfo = {};
		spotLightDescriptorInfo.buffer = _spotLightBuffer;
		spotLightDescriptorInfo.offset = 0;
		spotLightDescriptorInfo.range = sizeof(vk::wrappers::SpotLight) * _spotLights.size();
		
		//point lights
		VkDescriptorBufferInfo pointLightDescriptorInfo = {};
		pointLightDescriptorInfo.buffer = _pointLightBuffer;
		pointLightDescriptorInfo.offset = 0;
		pointLightDescriptorInfo.range = sizeof(vk::wrappers::PointLight) * _pointLights.size();
		
		//directional lights
		VkDescriptorBufferInfo directionalLightDescriptorInfo = {};
		directionalLightDescriptorInfo.buffer = _directionalLightBuffer;
		directionalLightDescriptorInfo.offset = 0;
		directionalLightDescriptorInfo.range = sizeof(vk::wrappers::DirectionalLight) * _directionalLights.size();

		VkDescriptorImageInfo shadowDescriptorInfo = {};
		shadowDescriptorInfo.sampler = shadowSampler;
		shadowDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		shadowDescriptorInfo.imageView = shadowFrameBuffer.depth.view;

		VkDescriptorBufferInfo lightViewMatrixDescriptorInfo = {};
		lightViewMatrixDescriptorInfo.buffer = lightViewMatrixBuffershading.buffer;
		lightViewMatrixDescriptorInfo.offset = 0;
		lightViewMatrixDescriptorInfo.range = sizeof(glm::mat4);
	
		VkDescriptorBufferInfo cameraEyeDescriptorInfo = {};
		cameraEyeDescriptorInfo.buffer = cameraEyeBuffer.buffer;
		cameraEyeDescriptorInfo.offset = 0;
		cameraEyeDescriptorInfo.range = sizeof(glm::vec4);


		//Set up the write descriptor sets
		std::array<VkWriteDescriptorSet, 9> descriptor_writes = {};
		//Binding 1: Position texture for offscreen rendering
		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[0].dstSet = _descriptorSets[i];
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].pImageInfo = &texDescriptorPosition;
		descriptor_writes[0].descriptorCount = 1;

		//Binding 2: normal texture for offscreen rendering
		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].dstSet = _descriptorSets[i];
		descriptor_writes[1].dstArrayElement = 0;
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].pImageInfo = &texDescriptorNormal;
		descriptor_writes[1].descriptorCount = 1;

		//Binding 3: albedo texture for offscreen rendering
		descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[2].dstSet = _descriptorSets[i];
		descriptor_writes[2].dstArrayElement = 0;
		descriptor_writes[2].dstBinding = 2;
		descriptor_writes[2].pImageInfo = &texDescriptorAlbedo;
		descriptor_writes[2].descriptorCount = 1;

		descriptor_writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[3].dstSet = _descriptorSets[i];
		descriptor_writes[3].dstBinding = 3;
		descriptor_writes[3].dstArrayElement = 0;
		descriptor_writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[3].descriptorCount = 1;
		descriptor_writes[3].pImageInfo = 0;
		descriptor_writes[3].pBufferInfo = &spotLightDescriptorInfo;
		
		descriptor_writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[4].dstSet = _descriptorSets[i];
		descriptor_writes[4].dstBinding = 4;
		descriptor_writes[4].dstArrayElement = 0;
		descriptor_writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[4].descriptorCount = 1;
		descriptor_writes[4].pImageInfo = 0;
		descriptor_writes[4].pBufferInfo = &pointLightDescriptorInfo;
		
		descriptor_writes[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[5].dstSet = _descriptorSets[i];
		descriptor_writes[5].dstBinding = 5;
		descriptor_writes[5].dstArrayElement = 0;
		descriptor_writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[5].descriptorCount = 1;
		descriptor_writes[5].pImageInfo = 0;
		descriptor_writes[5].pBufferInfo = &directionalLightDescriptorInfo;

		//Binding 3: albedo texture for offscreen rendering
		descriptor_writes[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[6].dstSet = _descriptorSets[i];
		descriptor_writes[6].dstArrayElement = 0;
		descriptor_writes[6].dstBinding = 6;
		descriptor_writes[6].pImageInfo = &shadowDescriptorInfo;
		descriptor_writes[6].descriptorCount = 1;

		descriptor_writes[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[7].dstSet = _descriptorSets[i];
		descriptor_writes[7].dstBinding = 7;
		descriptor_writes[7].dstArrayElement = 0;
		descriptor_writes[7].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[7].descriptorCount = 1;
		descriptor_writes[7].pImageInfo = 0;
		descriptor_writes[7].pBufferInfo = &lightViewMatrixDescriptorInfo;

		descriptor_writes[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[8].dstSet = _descriptorSets[i];
		descriptor_writes[8].dstBinding = 8;
		descriptor_writes[8].dstArrayElement = 0;
		descriptor_writes[8].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[8].descriptorCount = 1;
		descriptor_writes[8].pImageInfo = 0;
		descriptor_writes[8].pBufferInfo = &cameraEyeDescriptorInfo;

		//Update the descriptor set
		vkUpdateDescriptorSets(_renderer->GetVulkanDevice(), static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
	}




	//For rendering the quad
	VkDescriptorSetAllocateInfo descSetAllocInfo1{};
	descSetAllocInfo1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo1.descriptorPool = descriptorPool;
	descSetAllocInfo1.descriptorSetCount = 1;
	descSetAllocInfo1.pSetLayouts = &offScreenDescriptorSetLayout;



	
	//OffScreen (scene)
	vk::tools::ErrorCheck(vkAllocateDescriptorSets(_renderer->GetVulkanDevice(), &descSetAllocInfo1, &_models[0]->descriptorSet));

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = offScreenVertexUBOBuffer.buffer;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(uboVS);

	VkDescriptorBufferInfo dynamic_buffer_info = {};
	dynamic_buffer_info.buffer = dynamicUboBuffer.buffer;
	dynamic_buffer_info.offset = 0;
	dynamic_buffer_info.range = sizeof(glm::mat4);

	VkDescriptorBufferInfo material_data_info = {};
	material_data_info.buffer = materialIDBuffer.buffer;
	material_data_info.offset = 0;
	material_data_info.range = static_cast<uint32_t>(materialIDs.size()) * sizeof(materialInfo);

	uint32_t arraySize = _textures.size();

	std::vector<VkDescriptorImageInfo> diffuse_array_image_info = {};
	diffuse_array_image_info.resize(static_cast<uint32_t>(_textures.size()));

	for (size_t i = 0; i < diffuse_array_image_info.size(); i++)
	{
		diffuse_array_image_info[i].sampler = _textures[i].sampler;
		diffuse_array_image_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		diffuse_array_image_info[i].imageView = _textures[i].imageView;
	}
	std::vector<VkWriteDescriptorSet> descriptor_writes_model;
	//Binding 0: Vertex Shader UBO
	descriptor_writes_model.resize(4);
	descriptor_writes_model[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes_model[0].dstSet = _models[0]->descriptorSet;
	descriptor_writes_model[0].dstBinding = 0;
	descriptor_writes_model[0].dstArrayElement = 0;
	descriptor_writes_model[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes_model[0].descriptorCount = 1;
	descriptor_writes_model[0].pImageInfo = 0;
	descriptor_writes_model[0].pBufferInfo = &buffer_info;

	descriptor_writes_model[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes_model[1].dstSet = _models[0]->descriptorSet;
	descriptor_writes_model[1].dstBinding = 1;
	descriptor_writes_model[1].dstArrayElement = 0;
	descriptor_writes_model[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptor_writes_model[1].descriptorCount = 1;
	descriptor_writes_model[1].pImageInfo = 0;
	descriptor_writes_model[1].pBufferInfo = &dynamic_buffer_info;

	descriptor_writes_model[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes_model[2].dstSet = _models[0]->descriptorSet;
	descriptor_writes_model[2].dstBinding = 2;
	descriptor_writes_model[2].dstArrayElement = 0;
	descriptor_writes_model[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes_model[2].descriptorCount = static_cast<uint32_t>(diffuse_array_image_info.size());
	descriptor_writes_model[2].pImageInfo = diffuse_array_image_info.data();
	descriptor_writes_model[2].pBufferInfo = 0;

	descriptor_writes_model[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes_model[3].dstSet = _models[0]->descriptorSet;
	descriptor_writes_model[3].dstBinding = 3;
	descriptor_writes_model[3].dstArrayElement = 0;
	descriptor_writes_model[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes_model[3].descriptorCount = 1;
	descriptor_writes_model[3].pImageInfo = 0;
	descriptor_writes_model[3].pBufferInfo = &material_data_info;


	vkUpdateDescriptorSets(_renderer->GetVulkanDevice(), static_cast<uint32_t>(descriptor_writes_model.size()), descriptor_writes_model.data(), 0, nullptr);
	





	//Shadow Descriptor Set alloc info
	VkDescriptorSetAllocateInfo shadowDescSetAllocInfo = {};
	shadowDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	shadowDescSetAllocInfo.descriptorPool = descriptorPool;
	shadowDescSetAllocInfo.descriptorSetCount = 1;
	shadowDescSetAllocInfo.pSetLayouts = &shadowDescriptorSetLayout;

	vk::tools::ErrorCheck(vkAllocateDescriptorSets(_renderer->GetVulkanDevice(), &shadowDescSetAllocInfo, &shadowDescriptorSet));

	VkDescriptorBufferInfo shadow_buffer_info = {};
	shadow_buffer_info.buffer = offScreenVertexUBOBuffer.buffer;
	shadow_buffer_info.offset = 0;
	shadow_buffer_info.range = sizeof(uboVS);
	
	VkDescriptorBufferInfo shadow_dynamic_buffer_info = {};
	shadow_dynamic_buffer_info.buffer = dynamicUboBuffer.buffer;
	shadow_dynamic_buffer_info.offset = 0;
	shadow_dynamic_buffer_info.range = sizeof(glm::mat4);

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
	shadow_descriptor_writes_model[0].pBufferInfo = &shadow_buffer_info;

	shadow_descriptor_writes_model[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	shadow_descriptor_writes_model[1].dstSet = shadowDescriptorSet;
	shadow_descriptor_writes_model[1].dstBinding = 1;
	shadow_descriptor_writes_model[1].dstArrayElement = 0;
	shadow_descriptor_writes_model[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	shadow_descriptor_writes_model[1].descriptorCount = 1;
	shadow_descriptor_writes_model[1].pImageInfo = 0;
	shadow_descriptor_writes_model[1].pBufferInfo = &shadow_dynamic_buffer_info;

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

//Creates descriptor set layouts for Deferred rendering
void VulkanDeferredApplication::_CreateDescriptorSetLayout()
{
	//====Deferred Layouts====
	//Position layout binding (deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding position_layout_binding = {};
	position_layout_binding.binding = 0;
	position_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	position_layout_binding.descriptorCount = 1;
	position_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Normal layout binding (Deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding normal_layout_binding = {};
	normal_layout_binding.binding = 1;
	normal_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normal_layout_binding.descriptorCount = 1;
	normal_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//albedo layout binding (Deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding albedo_layout_binding = {};
	albedo_layout_binding.binding = 2;
	albedo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedo_layout_binding.descriptorCount = 1;
	albedo_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Point Light Layout binding (Deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding point_light_layout_binding = {};
	point_light_layout_binding.binding = 3; 
	point_light_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	point_light_layout_binding.descriptorCount = 1;
	point_light_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	//spot light layout binding (Deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding spot_light_layout_binding = {};
	spot_light_layout_binding.binding = 4;
	spot_light_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	spot_light_layout_binding.descriptorCount = 1;
	spot_light_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	//directional light layout binding (Deferred offscreen buffer sampler)
	VkDescriptorSetLayoutBinding directional_light_layout_binding = {};
	directional_light_layout_binding.binding = 5;
	directional_light_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	directional_light_layout_binding.descriptorCount = 1;
	directional_light_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding shadowmap_texture_layout_binding = {};
	shadowmap_texture_layout_binding.binding = 6;
	shadowmap_texture_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	shadowmap_texture_layout_binding.descriptorCount = 1;
	shadowmap_texture_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding light_matrix_layout_binding = {};
	light_matrix_layout_binding.binding = 7;
	light_matrix_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	light_matrix_layout_binding.descriptorCount = 1;
	light_matrix_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding camera_eye_layout_binding = {};
	camera_eye_layout_binding.binding = 8;
	camera_eye_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	camera_eye_layout_binding.descriptorCount = 1;
	camera_eye_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorSetLayoutBinding> deferredDescriptorSetLayoutBindings = {position_layout_binding, normal_layout_binding, albedo_layout_binding, point_light_layout_binding, spot_light_layout_binding, directional_light_layout_binding, shadowmap_texture_layout_binding, light_matrix_layout_binding, camera_eye_layout_binding };
	VkDescriptorSetLayoutCreateInfo deferredDescriptorLayoutCreateInfo = {};
	deferredDescriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	deferredDescriptorLayoutCreateInfo.pBindings = deferredDescriptorSetLayoutBindings.data();
	deferredDescriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(deferredDescriptorSetLayoutBindings.size());
	deferredDescriptorLayoutCreateInfo.flags = 0;

	vk::tools::ErrorCheck(vkCreateDescriptorSetLayout(_renderer->GetVulkanDevice(), &deferredDescriptorLayoutCreateInfo, nullptr, &deferredDescriptorSetLayout));

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(uint32_t);

	VkPipelineLayoutCreateInfo deferredPipelineLayoutCreateInfo = {};
	deferredPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	deferredPipelineLayoutCreateInfo.pSetLayouts = &deferredDescriptorSetLayout;
	deferredPipelineLayoutCreateInfo.setLayoutCount = 1;
	deferredPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	deferredPipelineLayoutCreateInfo.pushConstantRangeCount = 1;

	vk::tools::ErrorCheck(vkCreatePipelineLayout(_renderer->GetVulkanDevice(), &deferredPipelineLayoutCreateInfo, nullptr, &_pipelineLayout[PipelineType::deferred]));




	//====Offscreen Layout====
	//binding for the View and projection matrix
	VkDescriptorSetLayoutBinding ubo_layout_binding = {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//Dynamic Model layout binding for the model matrices
	VkDescriptorSetLayoutBinding dynamic_model_layout_binding = {};
	dynamic_model_layout_binding.binding = 1;
	dynamic_model_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	dynamic_model_layout_binding.descriptorCount = 1;
	dynamic_model_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//Texture for the model
	VkDescriptorSetLayoutBinding texture_layout_binding = {};
	texture_layout_binding.binding = 2;
	texture_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_layout_binding.descriptorCount = static_cast<uint32_t>(_textures.size());;
	texture_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding material_ID_layout_binding = {};
	material_ID_layout_binding.binding = 3;
	material_ID_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	material_ID_layout_binding.descriptorCount = 1;
	material_ID_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	std::vector<VkDescriptorSetLayoutBinding> offScreenDescriptorSetLayoutBindings = { ubo_layout_binding, dynamic_model_layout_binding, texture_layout_binding, material_ID_layout_binding };
	VkDescriptorSetLayoutCreateInfo offScreenDescriptorLayoutCreateInfo = {};
	offScreenDescriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	offScreenDescriptorLayoutCreateInfo.pBindings = offScreenDescriptorSetLayoutBindings.data();
	offScreenDescriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(offScreenDescriptorSetLayoutBindings.size());
	offScreenDescriptorLayoutCreateInfo.flags = 0;

	vk::tools::ErrorCheck(vkCreateDescriptorSetLayout(_renderer->GetVulkanDevice(), &offScreenDescriptorLayoutCreateInfo, nullptr, &offScreenDescriptorSetLayout));

	VkPipelineLayoutCreateInfo offScreenPipelineLayoutCreateInfo = {};
	offScreenPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	offScreenPipelineLayoutCreateInfo.pSetLayouts = &offScreenDescriptorSetLayout;
	offScreenPipelineLayoutCreateInfo.setLayoutCount = 1;

	vk::tools::ErrorCheck(vkCreatePipelineLayout(_renderer->GetVulkanDevice(), &offScreenPipelineLayoutCreateInfo, nullptr, &_pipelineLayout[PipelineType::offscreen]));




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

void VulkanDeferredApplication::_CreateOffScreenColorRenderPass()
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

void VulkanDeferredApplication::_CreateOffScreenColorPipeline()
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
	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::deferred];
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
	shaderStages[0] = loadShader("Shaders/Deferred/deferred.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/Deferred/deferred.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	shaderStages[1].pSpecializationInfo = &specializationInfo;

	VkPipelineVertexInputStateCreateInfo emptyVertexInputState = {};
	emptyVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	multisampleStateInfo.rasterizationSamples = sampleCount;
	pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;

	pipelineCreateInfo.layout = _pipelineLayout[PipelineType::deferred];

	vk::tools::ErrorCheck(vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphicsPipelines[PipelineType::offscreenColor]));


}

void VulkanDeferredApplication::_CreateOffScreenColorCommandBuffers()
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
	vkCmdResetQueryPool(offScreenColorCmdBuffers, DeferredShadeQueryPool, 0, 1);
	vkCmdResetQueryPool(offScreenColorCmdBuffers, DeferredShadeQueryPool, 1, 1);
	vkCmdWriteTimestamp(offScreenColorCmdBuffers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, DeferredShadeQueryPool, 0);
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


	vkCmdPushConstants(offScreenColorCmdBuffers, _pipelineLayout[PipelineType::deferred], VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &drawModeValue);
	vkCmdBindPipeline(offScreenColorCmdBuffers, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::offscreenColor]);
	vkCmdBindDescriptorSets(offScreenColorCmdBuffers, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::deferred], 0, 1, &_descriptorSets[0], 0, NULL);
	vkCmdBindVertexBuffers(offScreenColorCmdBuffers, 0, 1, &screenTarget->GetVertexBuffer()->buffer, offsets);
	vkCmdBindIndexBuffer(offScreenColorCmdBuffers, screenTarget->GetIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(offScreenColorCmdBuffers, screenTarget->GetIndexCount(), 1, 0, 0, 1);

	vkCmdEndRenderPass(offScreenColorCmdBuffers);
	vkCmdWriteTimestamp(offScreenColorCmdBuffers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, DeferredShadeQueryPool, 1);
	vk::tools::ErrorCheck(vkEndCommandBuffer(offScreenColorCmdBuffers));
}

//Runs the command buffer for the shadow render pass
void VulkanDeferredApplication::CreateShadowPassCommandBuffers()
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
	
	VkDeviceSize offsets[1] = { 0 };

	//Loop for each model in our scene
	for (size_t i = 0; i < _models.size(); i++)
	{
		//Dynamic offset to get the correct model matrix from the dynamic buffer
		uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);
		//binding descriptor sets and drawing model on the screen
		vkCmdBindPipeline(shadowCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::shadowMap]);
		vkCmdBindDescriptorSets(shadowCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::shadowMap], 0, 1, &shadowDescriptorSet, 1, &dynamicOffset);
		vkCmdBindVertexBuffers(shadowCmdBuffer, 0, 1, &_models[0]->model->GetVertexBuffer()->buffer, offsets);
		vkCmdBindIndexBuffer(shadowCmdBuffer, _models[0]->model->GetIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(shadowCmdBuffer, _models[0]->model->GetIndexCount(), 1, 0, 0, 0);
	}


	vkCmdEndRenderPass(shadowCmdBuffer);

	//End the command buffer drawing
	vk::tools::ErrorCheck(vkEndCommandBuffer(shadowCmdBuffer));
}

//Creates the deferred command buffers and runs an offscreen frame buffer render pass
void VulkanDeferredApplication::CreateDeferredCommandBuffers()
{
	//if the offscreen cmd buffer hasn't been initialised.
	if (offScreenCmdBuffer == VK_NULL_HANDLE)
	{
		//Create a single command buffer for the offscreen rendering
		VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.commandPool = _commandPool;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate_info.commandBufferCount = 1;

		vkAllocateCommandBuffers(_renderer->GetVulkanDevice(), &command_buffer_allocate_info, &offScreenCmdBuffer);
	}

	//Set up semaphore create info
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	//Create a signal semaphore for when the off screen rendering is complete (For pipeline ordering)
	vk::tools::ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreCreateInfo, nullptr, &offScreenSemaphore));

	//set up cmd buffer begin info
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	//Clear values for attachments in fragment shader
	std::array<VkClearValue, 4> clearValues;
	clearValues[0].color = { {  0.0f,0.0f,0.0f,0.0f } };
	clearValues[1].color = { {  0.0f,0.0f,0.0f,0.0f } };
	clearValues[2].color = { {  0.0f,0.0f,0.0f,0.0f } };
	clearValues[3].depthStencil = { 1.0f, 0 };

	//begin to set up the information for the render pass
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.framebuffer = deferredOffScreenFrameBuffer.frameBuffer;
	renderPassBeginInfo.renderPass = deferredOffScreenFrameBuffer.renderPass;
	renderPassBeginInfo.renderArea.extent.width = deferredOffScreenFrameBuffer.width;
	renderPassBeginInfo.renderArea.extent.height = deferredOffScreenFrameBuffer.height;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();


	//begin command buffer and start the render pass
	vk::tools::ErrorCheck(vkBeginCommandBuffer(offScreenCmdBuffer, &cmdBufferBeginInfo));
	vkCmdResetQueryPool(offScreenCmdBuffer, DeferredMRTQueryPool, 0, 1);
	vkCmdResetQueryPool(offScreenCmdBuffer, DeferredMRTQueryPool, 1, 1);
	vkCmdWriteTimestamp(offScreenCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, DeferredMRTQueryPool, 0);

	vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)deferredOffScreenFrameBuffer.width;
		viewport.height = (float)deferredOffScreenFrameBuffer.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);
	
		VkRect2D scissor = {};
		scissor.extent.width = deferredOffScreenFrameBuffer.width;
		scissor.extent.height = deferredOffScreenFrameBuffer.height;
		scissor.offset = { 0,0 };
		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);
		VkDeviceSize offsets[1] = { 0 };
		
		//Loop for each model in our scene
		for (size_t i = 0; i < _models.size(); i++)
		{
			//Dynamic offset to get the correct model matrix from the dynamic buffer
			uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);

			//binding descriptor sets and drawing model on the screen
			vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::offscreen]);
			vkCmdBindDescriptorSets(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::offscreen], 0, 1, &_models[0]->descriptorSet, 1, &dynamicOffset);
			vkCmdBindVertexBuffers(offScreenCmdBuffer, 0, 1, &_models[0]->model->GetVertexBuffer()->buffer, offsets);
			vkCmdBindIndexBuffer(offScreenCmdBuffer, _models[0]->model->GetIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexedIndirect(offScreenCmdBuffer, indirectCommandsBuffer.buffer, 0, indirectDrawCount, sizeof(VkDrawIndexedIndirectCommand));
		}


	vkCmdEndRenderPass(offScreenCmdBuffer);
	vkCmdWriteTimestamp(offScreenCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, DeferredMRTQueryPool, 1);
	//End the command buffer drawing
	vk::tools::ErrorCheck(vkEndCommandBuffer(offScreenCmdBuffer));

}

//Method to draw with the command buffers (Override)
void VulkanDeferredApplication::_CreateCommandBuffers()
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
	for (size_t i = 0; i < _drawCommandBuffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = _swapChainFramebuffers[i];
		
		vk::tools::ErrorCheck(vkBeginCommandBuffer(_drawCommandBuffers[i], &command_buffer_begin_info));
		vkCmdBeginRenderPass(_drawCommandBuffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)deferredOffScreenFrameBuffer.width;
			viewport.height = (float)deferredOffScreenFrameBuffer.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(_drawCommandBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.extent.width = deferredOffScreenFrameBuffer.width;
			scissor.extent.height = deferredOffScreenFrameBuffer.height;
			scissor.offset = { 0,0 };
			vkCmdSetScissor(_drawCommandBuffers[i], 0, 1, &scissor);


			VkDeviceSize offsets[] = { 0 };
			

			vkCmdPushConstants(_drawCommandBuffers[i], _pipelineLayout[PipelineType::deferred], VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &drawModeValue);
			vkCmdBindPipeline(_drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[PipelineType::deferred]);
			vkCmdBindDescriptorSets(_drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout[PipelineType::deferred], 0, 1, &_descriptorSets[i], 0, NULL);
			vkCmdBindVertexBuffers(_drawCommandBuffers[i], 0, 1, &screenTarget->GetVertexBuffer()->buffer, offsets);
			vkCmdBindIndexBuffer(_drawCommandBuffers[i], screenTarget->GetIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(_drawCommandBuffers[i], screenTarget->GetIndexCount(), 1, 0, 0, 1);
		

			imGui->DrawFrame(_drawCommandBuffers[i]);

		vkCmdEndRenderPass(_drawCommandBuffers[i]);
		//vkCmdEndRenderPass(_drawCommandBuffers[i]);

		vk::tools::ErrorCheck(vkEndCommandBuffer(_drawCommandBuffers[i]));
	}



}

//method to create vertex descriptions for the buffers 
void VulkanDeferredApplication::_CreateVertexDescriptions()
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

	vertices.attributeDescriptions[2].binding = 0;
	vertices.attributeDescriptions[2].location = 2;
	vertices.attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertices.attributeDescriptions[2].offset = offsetof(Vertex, normal);

	vertices.inputState = {};
	vertices.inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertices.inputState.flags = 0;
	vertices.inputState.pNext = 0;
	vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.attributeDescriptions.size());
	vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	vertices.inputState.vertexBindingDescriptionCount = 1;
	vertices.inputState.pVertexBindingDescriptions = &vertices.bindingDescriptions;
}

//Creates descriptor pool
void VulkanDeferredApplication::_CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 4> pool_sizes {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(20);
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = static_cast<uint32_t>(400);
	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[2].descriptorCount = static_cast<uint32_t>(_swapChainImages.size() * 2);
	pool_sizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	pool_sizes[3].descriptorCount = static_cast<uint32_t>(2);

	VkDescriptorPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_create_info.pPoolSizes = pool_sizes.data();
	pool_create_info.maxSets = static_cast<uint32_t>(_swapChainImages.size());

	vk::tools::ErrorCheck(vkCreateDescriptorPool(_renderer->GetVulkanDevice(), &pool_create_info, nullptr, &_descriptorPool));

	
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(200);
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = static_cast<uint32_t>(400);
	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[2].descriptorCount = static_cast<uint32_t>(11);
	pool_sizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	pool_sizes[3].descriptorCount = static_cast<uint32_t>(200);

	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_create_info.pPoolSizes = pool_sizes.data();
	pool_create_info.maxSets = static_cast<uint32_t>(1600);

	
	vk::tools::ErrorCheck(vkCreateDescriptorPool(_renderer->GetVulkanDevice(), &pool_create_info, nullptr, &descriptorPool));
}
