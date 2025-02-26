#include <vk_pipelines.h>
#include <fstream>

void PipelineBuilder::set_color_attachment_format(VkFormat format)
{
	// The color atttachment format specifies the data layout, component order, bit depth and encoding
	// of the image used as a color attachment in a render pass.
	_colorAttachmentFormat = format;

	// Connect the format to the renderInfo structure
	_renderInfo.colorAttachmentCount = 1;
	_renderInfo.pColorAttachmentFormats = &_colorAttachmentFormat;
}

void PipelineBuilder::set_depth_format(VkFormat format)
{
	_renderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::disable_depthtest()
{
	_depthStencil.depthTestEnable = VK_FALSE;
	_depthStencil.depthWriteEnable = VK_FALSE;
	_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	_depthStencil.depthBoundsTestEnable = VK_FALSE;
	_depthStencil.stencilTestEnable = VK_FALSE;
	_depthStencil.front = {};
	_depthStencil.back = {};
	_depthStencil.minDepthBounds = 0.f;
	_depthStencil.maxDepthBounds = 1.f;
}

void PipelineBuilder::disable_blending()
{
	// Blending is the process of combining the color output from the fragment shader (source)
	// with the color already in the framebuffer (destination).

	// Specify the color components written to the framebuffer (in this case RGBA, all channels)
	_colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	// Disable blending
	_colorBlendAttachment.blendEnable = VK_FALSE;
}

void PipelineBuilder::set_multisampling_none()
{
	// Right now, multisampling is simply disabled
	_multiSampling.sampleShadingEnable = VK_FALSE;
	// multisamlping defaulted to no multisamlping (1 sample per pixel)
	_multiSampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	_multiSampling.minSampleShading = 1.0f;
	_multiSampling.pSampleMask = nullptr;
	// no alpha to coverage either
	_multiSampling.alphaToCoverageEnable = VK_FALSE;
	_multiSampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
	_rasterizer.cullMode = cullMode;
	_rasterizer.frontFace = frontFace;
}

void PipelineBuilder::set_polygon_mode(VkPolygonMode polygonMode)
{
	// Controls things like wireframe, solid rendering, and point rendering
	_rasterizer.polygonMode = polygonMode;
	_rasterizer.lineWidth = 1.f;
}

void PipelineBuilder::set_input_topology(VkPrimitiveTopology topology)
{
	_inputAssembly.topology = topology;
	_inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
{
	_shaderStages.clear();

	_shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader)
	);

	_shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader)
	);
}

void PipelineBuilder::clear()
{
	_inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
	};

	_rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
	};

	_colorBlendAttachment = {};

	_multiSampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
	};

	_pipelineLayout = {};

	_depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
	};

	_renderInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO
	};

	_shaderStages.clear();
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device)
{
	// Make viewport state from our stored viewport and scissor
	// at the moment we wont support mulitple viewports or scissors
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Setup dummy color blending. We arent using transparent objects yet
	// the blending is just "no blend", but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &_colorBlendAttachment;

	// Completely clear VertexInputStateCreateInfo, as we have no need for it
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	// Create dynamic state info
	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
	};
	dynamicInfo.pDynamicStates = &state[0];
	dynamicInfo.dynamicStateCount = 2;

	// Build the actual pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
	};

	pipelineInfo.pNext = &_renderInfo;
	pipelineInfo.stageCount = (uint32_t)_shaderStages.size();
	pipelineInfo.pStages = _shaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multiSampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &_depthStencil;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.pDynamicState = &dynamicInfo;

	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&newPipeline) != VK_SUCCESS) 
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create pipeline");
		return VK_NULL_HANDLE;
	}

	return newPipeline;
}

bool vkutil::load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	// find what the size of the file is by looking up the location of the cursor
	// because the cursor is at the end, it gives the size directly in bytes
	size_t fileSize = (size_t)file.tellg();

	// spirv expects the buffer to be on uint32, so make sure to reserve an int
	// vector big enough for the entire file
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	// put the file cursor at the beginning
	file.seekg(0);

	// load the entire file into the buffer
	file.read((char*)buffer.data(), fileSize);

	// close the file
	file.close();

	// Create a new shader module
	VkShaderModuleCreateInfo shaderCreateInfo = {};
	shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderCreateInfo.pNext = nullptr;

	// codeSize has to be in bytes, so multiple the ints in the buffer by the size of
	// int to know the real size of the buffer
	shaderCreateInfo.codeSize = buffer.size() * sizeof(uint32_t);
	shaderCreateInfo.pCode = buffer.data();

	// Check that creation goes well
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return false;
	}

	*outShaderModule = shaderModule;

	return true;
}