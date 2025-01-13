// SDL_main should only be included from a single file
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>

#include "roguelike-x.h"

using namespace std;

void panic_and_exit(const char* error_message, ...);

int main(int argc, char** argv)
{
	// SDL_INIT_VIDEO = Initialize SDL's video subsytem.
	// This is largely abstracting window management from the underlying OS.
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != true)
	{
		std::cout << "Failed to initialize SDL!" << std::endl;
		exit(1);
	}

	SDL_SetAppMetadata("Roguelike-X", "1.0", NULL);

	auto main_window = SDL_CreateWindow("Roguelike-X", 800, 600, SDL_WINDOW_VULKAN);
	if (main_window == NULL) {
		panic_and_exit("Could not create window: %s\n", SDL_GetError());
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