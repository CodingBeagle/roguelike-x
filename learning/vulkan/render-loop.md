# Render Loop

- Render Fence
    - The CPU will wait until all commands in the command buffer have executed.
- Swapchain Semaphore
    - The queue will await execution of the command buffer until this is signaled (meaning the swapchain image is ready for use)
- Render Semaphore
    - Will be signaled once the command buffer has been executed.
    - Used by the VkQueuePresentKHR function to await presenting the image until rendering is done.