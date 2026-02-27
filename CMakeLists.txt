cmake_minimum_required(VERSION 3.21)
project(vk_hybrid_raytracer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(ENABLE_HARDWARE_RT "Enable Vulkan hardware ray tracing backend" ON)
option(ENABLE_SOFTWARE_RT "Enable software CPU ray tracing backend" ON)

if(NOT ENABLE_HARDWARE_RT AND NOT ENABLE_SOFTWARE_RT)
    message(FATAL_ERROR "At least one backend must be enabled")
endif()

add_executable(vk_hybrid_raytracer
    src/main.c
    src/app.c
    src/software_rt.c
    src/scene.c
    src/bvh.c
    src/vulkan_rt.c
)

target_include_directories(vk_hybrid_raytracer PRIVATE include)

target_compile_definitions(vk_hybrid_raytracer PRIVATE
    $<$<CONFIG:Debug>:RT_DEBUG=1>
    $<$<CONFIG:Release>:RT_RELEASE=1>
    $<$<BOOL:${ENABLE_HARDWARE_RT}>:ENABLE_HARDWARE_RT=1>
    $<$<BOOL:${ENABLE_SOFTWARE_RT}>:ENABLE_SOFTWARE_RT=1>
)

if(MSVC)
    target_compile_options(vk_hybrid_raytracer PRIVATE /W4)
else()
    target_compile_options(vk_hybrid_raytracer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ENABLE_HARDWARE_RT)
    find_package(Vulkan REQUIRED)
    target_link_libraries(vk_hybrid_raytracer PRIVATE Vulkan::Vulkan)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(vk_hybrid_raytracer PRIVATE m)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/raytracing.slang
               ${CMAKE_CURRENT_BINARY_DIR}/raytracing.slang COPYONLY)
