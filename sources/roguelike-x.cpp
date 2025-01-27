// SDL_main should only be included from a single file
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_log.h>

#include "roguelike-x.h"

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

using namespace std;

void panic_and_exit(const char* error_message, ...);

VkInstance vk_instance;
VkDebugUtilsMessengerEXT vk_debug_messenger;

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
	vkb::InstanceBuilder builder;

	// Make the Vulkan instance with basic debug features
	auto instance_build_result = builder.set_app_name("roguelike-x")
		.request_validation_layers(true)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_instance = instance_build_result.value();

	vk_instance = vkb_instance.instance;
	vk_debug_messenger = vkb_instance.debug_messenger;

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