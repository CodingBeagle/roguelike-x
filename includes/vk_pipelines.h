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

	void set_color_attachment_format(VkFormat format);
	void set_depth_format(VkFormat format);
	void disable_depthtest();
	void disable_blending();
	void set_multisampling_none();
	void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
	void set_polygon_mode(VkPolygonMode polygonMode);
	void set_input_topology(VkPrimitiveTopology topology);
	void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
	void clear();
	VkPipeline build_pipeline(VkDevice device);
};

namespace vkutil {
	bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
}