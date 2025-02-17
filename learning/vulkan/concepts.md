# Concepts

## Object Model

- Devices, queues and other entities are represented by **vulkan objects**.
- All objects are referred to by handles.
    - Two types of handles:
    - Dispatchable: Pointers to an opaque type.
    - Non-dispatchable: 64-bit integer type whos meaning is implementation-dependent.

### Object Lifetime

- *It is an applications responsibility to track the lifetime of Vulkan objects, to destroy them, and to not destroy them while still being used*.
- Objects are created by **vkCreate** and **vkAllocate**.
    - Objects allocated (rather than created) take resources from an existing pool object or memory heap, and when freed return resources to that pool or heap.
    - Creation is expected to be loq-frequency occurrenses during runtime, but allocation and freeing can occur at high frequency.
- Objects are destroyed by **vkDestroy** and **vkFree**.

## Execution Model

### Devices

- Vulkan exposes **devices** (like a GPU)
-- Each device has one or more **queues**, grouped into **families**.
-- Each **family** contains queues with different sets of functionality.

### Device Memory

Device memory is explicitly managed by the application.

A device can expose one or more heaps, representing different areas of memory.

### Queues

- Qeueus provide an interface to the execution engine of a device.
- **Commands** for these execution engines are recorded into **command buffers** ahead of execution.
    - Once submitted to a queue, command buffers will begin and complete execution without further intervention.
- Upon queue submission, a list of semaphores can be given upon which to wait before work begins.
- Upon queue submission, a list of semaphores can be given which will be signalled once work has completed.

## Synchronization

It is primarily the responsibility of the application to synchronize access to resources in Vulkan.

### Fences

- Synchronization primitive that can be used to insert a dependency from **a queue to the host**.
    - host = usually the CPU.
    - queue = the GPU.
- Two states:
    - Signaled & Unsignaled.
    - The GPU will signal the fence when a submitted set of commands completes execution.
- A fence can be awaited on the host, and can be signaled from a command on the queue (which is usually executed on the GPU).
- Can be reset on the host, so that it can be used to signal again.

### Semaphores

- Synchronization primitive that can be used to insert a dependency between queue operations or between a queue operation and the host.
    - Can for example be used to create a dependency between different commands in a queue.

## Instances

- No global state in Vulkan.
- All per-application state is stored in a **VkInstance**.
- Creating a *VkInstance* initializes the Vulkan library.

## Device

- Physical Device
    - Represents a single complete implementation of Vulkan available to the host
    - Usually in the form of a single GPU (on a desktop PC).
- Logical Device
    - Represents an instance of that implementation with its own state and resources independent of other logical devices.

## Swapchains

- A collection of images used for presenting rendered content to a display.
    - Acts as a buffer queue, frames rendered into images then presented sequentially.
- Coupled to a **VkSurfaceKHR** for presenting the images.

## Surfaces

When we want to render visual images to a window, for example in Windows, we need to render to a platform-specific surface within the window on the platform we are executing Vulkan on.

In order to interface with this platform-specific window, Vulkan uses the **VkSurfaceKHR** type, which acts as an abstract handle to an underlying platform-specific surface that can be used as a render target.

For windows, this will include underlying types such as a **HWND** (window handle) and **HINSTANCE** from the Windows API.

It is required to give a surface when you create a swapchain, for example.

```
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
```

## Command Buffers & Command Pools

- **Command Pools** are objects that allocate and manage command buffer memory.
    - You need a command pool in order to create multiple command buffers.
    - Command pools are created for specific queue families. Command buffers created from a pool will support submission to queues of this family.
- **Command Buffers**
    - Used for recording rendering, compute, and transfer commands before submitting them to a queue for execution.
    - They are submitted to a queue for execution.
    - Go through a lifecycle
        - Recording: In recording state, commands can be submitted to the queue.
        - Ending: After recording has finished, you put it in an ending state so that it can be executed.
        - Submitting: Submit to a queue.
        - Completion: Once executed, it returns to *executed* or *invalid*.
        - Resetting: Command buffers have to be reset in order to be recorded to again, or you can reset the entire pool.