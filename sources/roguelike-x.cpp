// SDL_main should only be included from a single file
#include <SDL3/SDL_main.h>

#include "roguelike-x.h"

using namespace std;

int main(int argc, char** argv)
{
	cout << "Hello SDL!" << endl;

	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != true)
	{
		cout << "Failed to initialize SDL!" << endl;
		SDL_Quit();
	}

	return 0;
}