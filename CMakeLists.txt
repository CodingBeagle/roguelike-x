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
    "sources/VkBootstrap.cpp"
    "includes/VkBootstrapDispatch.h"
    "sources/vk_pipelines.cpp"
    "includes/vk_pipelines.h"
    "sources/vk_initializers.cpp"
    "includes/vk_initializers.h"
    "includes/deletionqueue.h"
    "sources/deletionqueue.cpp"
    "includes/vk_mem_alloc.h" "includes/vk_types.h" "includes/vk_images.h" "sources/vk_images.cpp")

# Set C++ standard
set_target_properties(roguelike-x PROPERTIES CXX_STANDARD 20)

# Include SDL3 headers
target_include_directories(roguelike-x PRIVATE ${SDL3_SOURCE_DIR}/include)

# include VkBootstrap
target_include_directories(roguelike-x PRIVATE includes)

# Link libraries
target_link_libraries(roguelike-x PRIVATE SDL3::SDL3 Vulkan::Vulkan)

# Copy SDL3 DLL after build (Windows only)
add_custom_command(
    TARGET roguelike-x POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_RUNTIME_DLLS:roguelike-x>"
            "$<TARGET_FILE_DIR:roguelike-x>"
)

# Build shaders and output to output path
find_program(GLSL_VALIDATOR glslangValidator HINTS $ENV{VULKAN_SDK}/Bin $ENV{VULKAN_SDK}/Bin32)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/resources/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/resources/shaders/*.vert")

foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/resources/shaders/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    message(STATUS COMMAND ${GLSL_VALIDATOR} -v ${GLSL} -o ${SPIRV})
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(roguelike-x Shaders)

# Copy resources folder after build
add_custom_command(
    TARGET roguelike-x POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:roguelike-x>/resources"
    COMMENT "Copying resources folder to the output directory"
)