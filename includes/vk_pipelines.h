#pragma once

#include <vulkan/vulkan.h>

#include <SDL3/SDL_log.h>

#include <vk_initializers.h>

#include <vector>

class PipelineBuilder {
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multiSampling;
	VkPipelineLayout _pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo _depthStencil;
	VkPipelineRenderingCreateInfo _renderInfo;
	VkFormat _colorAttachmentFormat;

	PipelineBuilder() {
		clear();
	}

	void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);

	void clear();

	VkPipeline build_pipeline(VkDevice device);
};