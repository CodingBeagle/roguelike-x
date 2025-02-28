#include <deletionqueue.h>

void DeletionQueue::push_function(std::function<void()>&& function)
{
	deletors.push_back(function);
}

void DeletionQueue::flush()
{
	// Reverse iterate the deletion queue to execute all the functions
	for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
		(*it)(); // Call functors
	}

	deletors.clear();
}