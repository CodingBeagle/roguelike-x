#pragma once

#include <vulkan/vulkan.h>

namespace vkutil {
	void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
}