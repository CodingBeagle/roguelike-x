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
	}

	// Vulkan cleanup
	// The order in which you destroy resources matter.
	// A good rule of thumb is: Create resources in the opposite order they were created.
	// Physical devices cannot be destroyed as they are not a Vulkan resource, more a handle to the GPU in the system.

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

void panic_and_exit(const char* error_message, ...) {
	SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Application is panicking and exiting!");
	SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, error_message);
	exit(1);
}