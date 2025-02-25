#include <vector>

// SDL_main should only be included from a single file
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_vulkan.h>

#include "roguelike-x.h"

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

using namespace std;

void panic_and_exit(const char* error_message, ...);
void vk_check(VkResult vkResult);

struct FrameData {
	VkCommandPool commandPool;
	VkCommandBuffer main_command_buffer;
	VkSemaphore swapchain_semaphore;
	VkSemaphore render_semaphore;
	VkFence render_fence;
};

constexpr unsigned int FRAME_OVERLAP = 2;

VkInstance vk_instance;
VkDebugUtilsMessengerEXT vk_debug_messenger;

VkSurfaceKHR vk_surface;

VkDevice vk_device;
VkPhysicalDevice vk_physical_device;

VkSwapchainKHR vk_swapchain;
VkFormat vk_swapchain_image_format;

std::vector<VkImage> vk_swapchain_images;
std::vector<VkImageView> vk_swapchain_imageviews;
VkExtent2D vk_swapchain_extent;

FrameData frames[FRAME_OVERLAP];
int frame_number{ 0 };
FrameData& get_current_frame() { return frames[frame_number % FRAME_OVERLAP]; }

VkQueue graphics_queue;
uint32_t graphics_queue_family;

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);
void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);
VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);

int main(int argc, char** argv)
{
	// SDL_INIT_VIDEO = Initialize SDL's video subsytem.
	// This is largely abstracting window management from the underlying OS.
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != true)
	{
		std::cout << "Failed to initialize SDL!" << std::endl;
		exit(1);
	}

	auto sdl_version = SDL_GetVersion();
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Initialized SDL with version: %i\n", sdl_version);

	SDL_SetAppMetadata("Roguelike-X", "1.0", NULL);

	// Creating a window with SDL_WINDOW_VULKAN means that the corresponding LoadLibrary function will be called.
	// The corresponding UnloadLibrary function will also be called by SDL_DestroyWindow()
	auto main_window = SDL_CreateWindow("Roguelike-X", 800, 600, SDL_WINDOW_VULKAN);
	if (main_window == NULL) {
		panic_and_exit("Could not create window: %s\n", SDL_GetError());
	}

	// Initialize Vulkan
	// Make the Vulkan instance with basic debug features
	vkb::InstanceBuilder builder;

	auto instance_build_result = builder.set_app_name("roguelike-x")
		.request_validation_layers(true)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_instance = instance_build_result.value();

	vk_instance = vkb_instance.instance;
	vk_debug_messenger = vkb_instance.debug_messenger;

	// A Surface represents and abstract handle to a native platform window which can be rendered to.
	// We create a surface for our SDL window, which we will render to.
	SDL_Vulkan_CreateSurface(main_window, vk_instance, nullptr, &vk_surface);

	// Vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features_13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features_13.dynamicRendering = true;
	features_13.synchronization2 = true;

	// Vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features_12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features_12.bufferDeviceAddress = true;
	features_12.descriptorIndexing = true;

	// We use VkBootstrap to select a GPU
	// We want a GPU that can write to the SDL surface and supports Vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ vkb_instance };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features_13)
		.set_required_features_12(features_12)
		.set_surface(vk_surface)
		.select()
		.value();

	// Create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	vkb::Device vkbDevice = deviceBuilder.build().value();

	vk_device = vkbDevice.device;
	vk_physical_device = physicalDevice.physical_device;

	// Create Vulkan swapchain
	vkb::SwapchainBuilder swapchainBuilder{
		vk_physical_device,
		vk_device,
		vk_surface
	};

	vk_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{
			.format = vk_swapchain_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		// Use vsync present mode
		// This will limit FPS to the refresh rate of the monitor
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(800, 600)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	vk_swapchain_extent = vkbSwapchain.extent;
	vk_swapchain = vkbSwapchain.swapchain;
	vk_swapchain_images = vkbSwapchain.get_images().value();
	vk_swapchain_imageviews = vkbSwapchain.get_image_views().value();

	// Get queue that supports all types of commands
	graphics_queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphics_queue_family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	// Initialize command structures
	// Create a command pool for commands submitted to the graphics queue
	// We also want the pool to allow for resetting of individual command buffers
	
	// Notice that for Vulkan, it is a good practice to start by initializing the entire Vk struct to zero with {}
	// This ensures that the struct is in a relatively safe state for all its members.
	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	// Each command buffer created can be reset independently
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	// The command pool will create command buffers that are compatible with the "graphics" family type of queues
	commandPoolInfo.queueFamilyIndex = graphics_queue_family;

	// Create a command pool for each frame
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		vk_check(vkCreateCommandPool(vk_device, &commandPoolInfo, nullptr, &frames[i].commandPool));

		// Allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo {};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.pNext = nullptr;
		cmdAllocInfo.commandPool = frames[i].commandPool;
		cmdAllocInfo.commandBufferCount = 1;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		vk_check(vkAllocateCommandBuffers(vk_device, &cmdAllocInfo, &frames[i].main_command_buffer));
	}

	// Create synchronization structures for our frame data structs
	// One fence to control when the GPU has finished rendering the frame
	// 2 semaphores to synchronize rendering with swapchain
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		vk_check(vkCreateFence(vk_device, &fenceCreateInfo, nullptr, &frames[i].render_fence));

		vk_check(vkCreateSemaphore(vk_device, &semaphoreCreateInfo, nullptr, &frames[i].swapchain_semaphore));
		vk_check(vkCreateSemaphore(vk_device, &semaphoreCreateInfo, nullptr, &frames[i].render_semaphore));
	}

	// Game Loop
	bool should_quit = false;
	while (!should_quit) {
		// SDL_PollEvent is the favored way of receiving system events since it can be done from the main loop
		// without suspending / blocking it while waiting for an event to be posted.
		SDL_Event sdl_event;

		// It's a common practice to fully process the event queue once every frame before updating the game's state
		while (SDL_PollEvent(&sdl_event)) {
			switch (sdl_event.type) {
				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
					should_quit = true;
					break;
				default:
					break;
			}
		}

		// Drawing
		// Wait until the GPU has finished rendering the last frame. Timeout of 1 second
		vk_check(vkWaitForFences(vk_device, 1, &get_current_frame().render_fence, true, 1000000000));

		// Fences have to be reset between uses, you can't use the same fence on multiple GPU commands without resetting
		vk_check(vkResetFences(vk_device, 1, &get_current_frame().render_fence));

		// Request image from the swapchain to draw to
		// vkAcquireNextImageKHR will request an image index from the swapchain.
		// If the swapchain doesn't have an image we can use, it will block the thread with a maximum timeout.
		// We pass the frames swapchain semaphore so we can use it so sync with other operations later.
		// The Semaphore will be signalled once the image is acquired.
		// Even though an image index is returned at the time of this call, it might still not be ready for use.
		// By providing a semaphore, we can use this semaphore as a wait signal when submitting the command buffer to the queue,
		// Which will guarentee that the command buffer will first start execution once the swapchain image is actually ready for use.
		uint32_t swapchain_image_index;
		vk_check(vkAcquireNextImageKHR(
			vk_device,
			vk_swapchain,
			1000000000,
			get_current_frame().swapchain_semaphore,
			nullptr,
			&swapchain_image_index));

		// Record rendering commands
		VkCommandBuffer cmd = get_current_frame().main_command_buffer;

		// A command buffer has to be reset before we can use it again
		vk_check(vkResetCommandBuffer(cmd, 0));

		// Begin the command buffer recording.
		// We will use this command buffer exactly once, which we will let Vulkan know
		VkCommandBufferBeginInfo cmd_begin_info{};
		cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmd_begin_info.pNext = nullptr;
		cmd_begin_info.pInheritanceInfo = nullptr;
		cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		// Start command buffer recording
		vk_check(vkBeginCommandBuffer(cmd, &cmd_begin_info));

		// Make the swapchain image into writeable mode before rendering
		// VK_IMAGE_LAYOUT_UNDEFINED = "we dont care" layout. You can use it to indicate you don't care about the current data
		// in the image, and that you are fine with the GPU destroying it.
		// VK_IMAGE_LAYOUT_GENERAL = General purpose layout allowing reading and writing from the image.
		transition_image(cmd, vk_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		// Make a clear-color from frame number. This will flash with a 120 frame period.
		VkClearColorValue clearValue;
		float flash = std::abs(std::sin(frame_number / 120.f));
		clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

		VkImageSubresourceRange clearRange = image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

		// Clear image
		vkCmdClearColorImage(cmd, vk_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

		// Make the swapchain image into presentable mode
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = The only image layout the swapchain allows for presenting to the screen.
		transition_image(cmd, vk_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		// Finalize the command buffer (we can no longer add commands, but it can now be executed)
		vk_check(vkEndCommandBuffer(cmd));

		// Prepare the submission to the queue.
		// We want to wait on the presentSemaphore, as that semaphore is signaled when the swapchain is ready
		// We will signal the renderSemaphore, to singal that rendering has finished
		VkCommandBufferSubmitInfo cmdInfo = command_buffer_submit_info(cmd);

		VkSemaphoreSubmitInfo waitInfo = semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame().swapchain_semaphore);
		VkSemaphoreSubmitInfo signalInfo = semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame().render_semaphore);

		VkSubmitInfo2 submit = submit_info(&cmdInfo, &signalInfo, &waitInfo);

		// Submit command buffer to the queue and execute it
		// renderFence will now block until the graphic commands finish execution
		vk_check(vkQueueSubmit2(graphics_queue, 1, &submit, get_current_frame().render_fence));

		// Prepare present
		// This will put the image we just rendered to into the visible window.
		// We want to wait on the renderSemaphore for that,
		// As its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;

		presentInfo.pSwapchains = &vk_swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &get_current_frame().render_semaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchain_image_index;

		vk_check(vkQueuePresentKHR(graphics_queue, &presentInfo));

		// increase the number of frames drawn
		frame_number++;
	}

	// Vulkan cleanup
	// The order in which you destroy resources matter.
	// A good rule of thumb is: Create resources in the opposite order they were created.
	// Physical devices cannot be destroyed as they are not a Vulkan resource, more a handle to the GPU in the system.

	// Wait for the GPU to stop doing its thing
	vkDeviceWaitIdle(vk_device);

	// Destroy command pool
	// Destroying the command pool will destroy associated command buffers
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		vkDestroyCommandPool(vk_device, frames[i].commandPool, nullptr);

		// Destroy sync objects
		vkDestroyFence(vk_device, frames[i].render_fence, nullptr);
		vkDestroySemaphore(vk_device, frames[i].render_semaphore, nullptr);
		vkDestroySemaphore(vk_device, frames[i].swapchain_semaphore, nullptr);
	}

	// Destroy swapchain
	vkDestroySwapchainKHR(vk_device, vk_swapchain, nullptr);
	for (int i = 0; i < vk_swapchain_imageviews.size(); i++) {
		vkDestroyImageView(vk_device, vk_swapchain_imageviews[i], nullptr);
	}

	// Destroy surface
	vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);

	// Destroy device
	vkDestroyDevice(vk_device, nullptr);

	// Destroy debug messenger
	vkb::destroy_debug_utils_messenger(vk_instance, vk_debug_messenger);

	// Destroy instance
	vkDestroyInstance(vk_instance, nullptr);

	// Close and destroy the window
	SDL_DestroyWindow(main_window);

	// Clean up SDL allocations
	// Also revert display resolution back to what the user expects (if you changed it during the game)
	SDL_Quit();

	return 0;
}

// TODO: Perhaps make a safer escape that first attempts to clean up vulkan resources
void panic_and_exit(const char* error_message, ...) 
{
	SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Application is panicking and exiting!");
	SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, error_message);
	exit(1);
}

void vk_check(VkResult vkResult) 
{
	if (vkResult != VK_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Detected vulkan error: %i", vkResult);
		exit(1);
	}
}

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) 
{
	VkImageMemoryBarrier2 image_barrier{};
	image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	image_barrier.pNext = nullptr;
	
	// Specify the pipeline stages where memory operations must complete before the transition
	// VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT = All commands in the pipeline should be completed.
	image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	// Any previous write operations have to complete before transitioning the image.
	image_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;

	// After transition, any command can use the image
	image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	// After transition, both read and write access is allowed
	image_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	// Describe the transition
	image_barrier.oldLayout = currentLayout;
	image_barrier.newLayout = newLayout;

	VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ?
		VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	image_barrier.subresourceRange = image_subresource_range(aspectMask);
	image_barrier.image = image;

	VkDependencyInfo depInfo{};
	depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.pNext = nullptr;

	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &image_barrier;

	vkCmdPipelineBarrier2(cmd, &depInfo);
}

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask) 
{
	VkImageSubresourceRange subImage{};
	subImage.aspectMask = aspectMask;
	subImage.baseMipLevel = 0;
	subImage.levelCount = VK_REMAINING_MIP_LEVELS;
	subImage.baseArrayLayer = 0;
	subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

	return subImage;
}

VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) 
{
	VkSemaphoreSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.semaphore = semaphore;
	submitInfo.stageMask = stageMask;
	submitInfo.deviceIndex = 0;
	submitInfo.value = 1;

	return submitInfo;
}

VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd) 
{
	VkCommandBufferSubmitInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	info.pNext = nullptr;
	info.commandBuffer = cmd;
	info.deviceMask = 0;

	return info;
}

VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd,
	VkSemaphoreSubmitInfo* signalSemaphoreInfo,
	VkSemaphoreSubmitInfo* waitSemaphoreInfo) 
{
	VkSubmitInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	info.pNext = nullptr;

	info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
	info.pWaitSemaphoreInfos = waitSemaphoreInfo;

	info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
	info.pSignalSemaphoreInfos = signalSemaphoreInfo;

	info.commandBufferInfoCount = 1;
	info.pCommandBufferInfos = cmd;

	return info;
}