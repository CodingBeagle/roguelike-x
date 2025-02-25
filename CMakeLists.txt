cmake_minimum_required(VERSION 3.15)
project(roguelike-x VERSION 1.0 LANGUAGES CXX)

# Enable Hot Reload for MSVC compilers
if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT EditAndContinue)
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Use FetchContent to download SDL3 if not found
include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
)

FetchContent_MakeAvailable(SDL3)

# Add executable
add_executable(roguelike-x
    "sources/roguelike-x.cpp"
    "sources/roguelike-x.h"
    "sources/VkBootstrap.cpp"
    "includes/VkBootstrapDispatch.h"
 "sources/vk_pipelines.cpp" "includes/vk_pipelines.h" "sources/vk_initializers.cpp" "includes/vk_initializers.h")

# Set C++ standard
set_target_properties(roguelike-x PROPERTIES CXX_STANDARD 20)

# Include SDL3 headers
target_include_directories(roguelike-x PRIVATE ${SDL3_SOURCE_DIR}/include)

# include VkBootstrap
target_include_directories(roguelike-x PRIVATE includes)

# Link libraries
target_link_libraries(roguelike-x PRIVATE SDL3::SDL3 Vulkan::Vulkan)

# Copy SDL3 DLL after build (Windows only)
if(WIN32)
    add_custom_command(
        TARGET roguelike-x POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_RUNTIME_DLLS:roguelike-x>"
                "$<TARGET_FILE_DIR:roguelike-x>"
    )
endif()
