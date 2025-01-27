# CMakeList.txt : CMake project for roguelike-x, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("roguelike-x")

# Link SDL3
add_library(SDL3::SDL3 SHARED IMPORTED)

# Specify where the .dll is located
# And where the .lib (import library) is located
set_target_properties(SDL3::SDL3 PROPERTIES
	IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/libs/sdl3/SDL3.dll"
	IMPORTED_IMPLIB "${CMAKE_CURRENT_LIST_DIR}/libs/sdl3/SDL3.lib")

target_include_directories(SDL3::SDL3 INTERFACE "${CMAKE_CURRENT_LIST_DIR}/includes")

# Link Vulkan
find_package(Vulkan REQUIRED)

# Add source to this project's executable.
add_executable (roguelike-x "sources/roguelike-x.cpp" "sources/roguelike-x.h"  "includes/VkBootstrap.h" "sources/VkBootstrap.cpp" "includes/VkBootstrapDispatch.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET roguelike-x PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(roguelike-x PRIVATE SDL3::SDL3)
target_link_libraries(roguelike-x PRIVATE Vulkan::Vulkan)

# Copy the DLL to the .EXE output dir so that Windows can load the runtime
add_custom_command(
    TARGET roguelike-x POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_LIST_DIR}/libs/sdl3/SDL3.dll"
            "$<TARGET_FILE_DIR:roguelike-x>"
    COMMENT "Copying SDL3.dll to output directory"
)