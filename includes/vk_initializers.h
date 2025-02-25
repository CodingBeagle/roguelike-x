#pragma once

#include <vulkan/vulkan.h>

namespace vkinit {
	VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");
}