#include "ImGUIInterface.h"

ImGUIInterface::ImGUIInterface(Renderer * renderer)
{
	pRenderer = renderer;
	ImGui::CreateContext();
}

ImGUIInterface::~ImGUIInterface()
{
	ImGui::DestroyContext();
}

void ImGUIInterface::init(float width, float height)
{
	// Color scheme
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 1.0f, 0.6f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 1.0f, 0.8f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	// Dimensions
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(width, height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

void ImGUIInterface::initResources(VkRenderPass renderPass, VkQueue copyQueue)
{
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* fontData;
	int texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
	VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.extent.width = texWidth;
	imageInfo.extent.height = texHeight;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vk::tools::ErrorCheck(vkCreateImage(pRenderer->GetVulkanDevice(), &imageInfo, nullptr, &fontImage));
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(pRenderer->GetVulkanDevice(), fontImage, &memReqs);
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = pRenderer->_GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vk::tools::ErrorCheck(vkAllocateMemory(pRenderer->GetVulkanDevice(), &memAllocInfo, nullptr, &fontMemory));
	vk::tools::ErrorCheck(vkBindImageMemory(pRenderer->GetVulkanDevice(), fontImage, fontMemory, 0));

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.image = fontImage;
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;
	vk::tools::ErrorCheck(vkCreateImageView(pRenderer->GetVulkanDevice(), &viewInfo, nullptr, &fontView));


	//Staging buffer and copy commands

	vk::wrappers::Buffer stagingBuffer;
	pRenderer->_CreateBuffer(uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);


	stagingBuffer.map();
	memcpy(stagingBuffer.mapped, fontData, uploadSize);
	stagingBuffer.unmap();

	// Copy buffer data to font image
	VkCommandBuffer copyCmd;
	VkCommandBufferAllocateInfo  cmdbufAlocateInfo = {};
	cmdbufAlocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdbufAlocateInfo.commandPool = pRenderer->GetVulkanCommandPool();
	cmdbufAlocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdbufAlocateInfo.commandBufferCount = 1;

	vk::tools::ErrorCheck(vkAllocateCommandBuffers(pRenderer->GetVulkanDevice(), &cmdbufAlocateInfo, &copyCmd));
	VkCommandBufferBeginInfo cmdbufInfo = {};
	cmdbufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vk::tools::ErrorCheck(vkBeginCommandBuffer(copyCmd, &cmdbufInfo));

	vk::tools::setImageLayout(copyCmd, fontImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);


	// Copy
	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = texWidth;
	bufferCopyRegion.imageExtent.height = texHeight;
	bufferCopyRegion.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(
		copyCmd,
		stagingBuffer.buffer,
		fontImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion
	);

	// Prepare for shader read
	vk::tools::setImageLayout(copyCmd, fontImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	if (copyCmd == VK_NULL_HANDLE)
	{
		return;
	}

	vk::tools::ErrorCheck(vkEndCommandBuffer(copyCmd));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	VkFence fence;
	vk::tools::ErrorCheck(vkCreateFence(pRenderer->GetVulkanDevice(), &fenceInfo, nullptr, &fence));

	// Submit to the queue
	vk::tools::ErrorCheck(vkQueueSubmit(copyQueue, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	vk::tools::ErrorCheck(vkWaitForFences(pRenderer->GetVulkanDevice(), 1, &fence, VK_TRUE, 100000000000));

	vkDestroyFence(pRenderer->GetVulkanDevice(), fence, nullptr);

	stagingBuffer.destroy();

	//font texture
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vk::tools::ErrorCheck(vkCreateSampler(pRenderer->GetVulkanDevice(), &samplerInfo, nullptr, &sampler));


	//Descriptor pool
	std::vector<VkDescriptorPoolSize> poolSizes;

	//Creates a descriptor pool with sizes for each value
	VkDescriptorPoolSize poolSize = {};
	poolSize.descriptorCount = 1;
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes.push_back(poolSize);

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.maxSets = 2;
	vk::tools::ErrorCheck(vkCreateDescriptorPool(pRenderer->GetVulkanDevice(), &descriptorPoolCreateInfo, nullptr, &descriptorPool));

	//Descriptor set layouts and sets
	//Creates a layout binding for the pixel shader to take in a texture
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	setLayoutBindings = { layoutBinding };


	//Create a layout using these bindings
	VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {};
	descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	descriptorLayoutInfo.pBindings = setLayoutBindings.data();
	descriptorLayoutInfo.flags = 0;
	vk::tools::ErrorCheck(vkCreateDescriptorSetLayout(pRenderer->GetVulkanDevice(), &descriptorLayoutInfo, nullptr, &descriptorSetLayout));


	//Descriptor Set
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	//Start allocating descriptor sets
	vk::tools::ErrorCheck(vkAllocateDescriptorSets(pRenderer->GetVulkanDevice(), &allocInfo, &descriptorSet));

	VkDescriptorImageInfo fontDescriptor = {};
	fontDescriptor.imageView = fontView;
	fontDescriptor.sampler = sampler;
	fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


	std::array<VkWriteDescriptorSet, 1> writeDescriptorSets;
	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[0].dstSet = descriptorSet;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].dstBinding = 0;
	writeDescriptorSets[0].pImageInfo = &fontDescriptor;
	writeDescriptorSets[0].descriptorCount = 1;

	vkUpdateDescriptorSets(pRenderer->GetVulkanDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);



	//Create Pipeline 
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	vk::tools::ErrorCheck(vkCreatePipelineCache(pRenderer->GetVulkanDevice(), &pipelineCacheCreateInfo, nullptr, &pipelineCache));

	//pipeline layout push constants (For image changing)
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.size = sizeof(PushConstBlock);
	pushConstantRange.offset = 0;

	//Creates the pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	vk::tools::ErrorCheck(vkCreatePipelineLayout(pRenderer->GetVulkanDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));


	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.flags = 0;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

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
	colorBlendAttachmentState.blendEnable = VK_TRUE;
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo blendStateInfo = {};
	blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendStateInfo.attachmentCount = 1;
	blendStateInfo.pAttachments = &colorBlendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_FALSE;
	depthStencilState.depthWriteEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
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
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pColorBlendState = &blendStateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();

	std::array<VkVertexInputBindingDescription, 1> vertexInputBindings;
	vertexInputBindings[0].binding = 0;
	vertexInputBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBindings[0].stride = sizeof(ImDrawVert);

	std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributes;
	vertexInputAttributes[0].binding = 0;
	vertexInputAttributes[0].location = 0;
	vertexInputAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertexInputAttributes[0].offset = offsetof(ImDrawVert, pos);

	vertexInputAttributes[1].binding = 0;
	vertexInputAttributes[1].location = 1;
	vertexInputAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertexInputAttributes[1].offset = offsetof(ImDrawVert, uv);

	vertexInputAttributes[2].binding = 0;
	vertexInputAttributes[2].location = 2;
	vertexInputAttributes[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	vertexInputAttributes[2].offset = offsetof(ImDrawVert, col);

	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

	pipelineCreateInfo.pVertexInputState = &vertexInputState;

	shaderStages[0] = loadShader("Shaders/ImGui/ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("Shaders/ImGui/ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	vkCreateGraphicsPipelines(pRenderer->GetVulkanDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline);


}


void ImGUIInterface::newFrame(double frameTimerMRT, double frameTimerShading,  bool updateFrameGraph)
{
	ImGui::NewFrame();


	ImVec4 clear_color = ImColor(114, 144, 154);

	//ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));
	std::string cameraActive = "Camera Active: " + std::string(uiSettings.cameraActive == true ? "TRUE" : "FALSE");

	// Update frame time display
	if (updateFrameGraph && frameTimerMRT != 0.0f) {
		std::rotate(uiSettings.frameTimesMRT.begin(), uiSettings.frameTimesMRT.begin() + 1, uiSettings.frameTimesMRT.end());
		float frameTime = 1000.0f / (frameTimerMRT);
		uiSettings.frameTimesMRT.back() = frameTime;
		if (frameTime < uiSettings.frameTimeMinMRT && frameTime != 0.0f) {
			uiSettings.frameTimeMinMRT = frameTime;
		}
		if (frameTime > uiSettings.frameTimeMaxMRT && frameTime != 0.0f) {
			uiSettings.frameTimeMaxMRT = frameTime;
		}
	}

	// Update frame time display
	if (updateFrameGraph && frameTimerShading != 0.0f) {
		std::rotate(uiSettings.frameTimesShading.begin(), uiSettings.frameTimesShading.begin() + 1, uiSettings.frameTimesShading.end());
		float frameTime = 1000.0f / (frameTimerShading);
		uiSettings.frameTimesShading.back() = frameTime;
		if (frameTime < uiSettings.frameTimeMinShading && frameTime != 0.0f) {
			uiSettings.frameTimeMinShading = frameTime;
		}
		if (frameTime > uiSettings.frameTimeMaxShading && frameTime != 0.0f) {
			uiSettings.frameTimeMaxShading = frameTime;
		}
	}

	ImGui::Begin("Vulkan Deferred Rendering settings");

	ImGui::Checkbox("Render models", &uiSettings.displayModels);
	ImGui::Text(cameraActive.c_str());
	ImGui::Spacing();

	ImGui::Text(uiSettings.modelName.c_str());
	ImGui::Text("Vertices: %i   Indices %i    triangles %i", uiSettings.vertices, uiSettings.indices, uiSettings.face);
	ImGui::PlotLines("MRT frame time", &uiSettings.frameTimesMRT[0], 50, 0, "", uiSettings.frameTimeMinMRT, uiSettings.frameTimeMaxMRT, ImVec2(0, 80));
	ImGui::PlotLines("Shading frame time", &uiSettings.frameTimesShading[0], 50, 0, "", uiSettings.frameTimeMinShading, uiSettings.frameTimeMaxShading, ImVec2(0, 80));

	ImGui::End();



	// Render to generate draw buffers
	ImGui::Render();
}

void ImGUIInterface::UpdateImGuiInformation(bool cameraActive)
{
	uiSettings.cameraActive = cameraActive;
}

void ImGUIInterface::updateBuffers()
{
	ImDrawData* imDrawData = ImGui::GetDrawData();

	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
		return;
	}

	// Update buffers only if vertex or index count has been changed compared to current buffer size

	// Vertex buffer
	if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
		vertexBuffer.unmap();
		vertexBuffer.destroy();
		pRenderer->_CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexBuffer);
		vertexCount = imDrawData->TotalVtxCount;
		vertexBuffer.unmap();
		vertexBuffer.map();
	}

	// Index buffer
	VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
	if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {

		indexBuffer.unmap();
		indexBuffer.destroy();
		pRenderer->_CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &indexBuffer);
		indexCount = imDrawData->TotalIdxCount;
		indexBuffer.map();
	}

	// Upload data
	ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer.mapped;
	ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer.mapped;

	for (int n = 0; n < imDrawData->CmdListsCount; n++) {
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	vertexBuffer.flush();
	indexBuffer.flush();
}

void ImGUIInterface::DrawFrame(VkCommandBuffer commandBuffer)
{
	ImGuiIO& io = ImGui::GetIO();

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkViewport viewport = {};
	viewport.width = ImGui::GetIO().DisplaySize.x;
	viewport.height = ImGui::GetIO().DisplaySize.y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	// UI scale and translate via push constants
	pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
	pushConstBlock.translate = glm::vec2(-1.0f);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

	// Render commands
	ImDrawData* imDrawData = ImGui::GetDrawData();
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;

	if (imDrawData->CmdListsCount > 0) {

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				VkRect2D scissorRect;
				scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
				scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
				scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
				vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}
}

//loads a shader file
VkPipelineShaderStageCreateInfo ImGUIInterface::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = vk::tools::loadShader(fileName.c_str(), pRenderer->GetVulkanDevice());
	shaderStage.pName = "main"; // todo : make param
	assert(shaderStage.module != VK_NULL_HANDLE);
	return shaderStage;
}
