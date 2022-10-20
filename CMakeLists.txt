cmake_minimum_required (VERSION 3.12)
project (Floof)
set(CMAKE_CXX_STANDARD 20)

add_executable(Floof
    Source/Floof.cpp
    Source/Floof.h
    Source/Renderer/VulkanRenderer.h
    Source/Renderer/VulkanRenderer.cpp
    Source/Renderer/VulkanAllocator.cpp
    Source/Application.h
    Source/Application.cpp
    Source/Timer.h
    Source/ObjLoader.h
    Source/ObjLoader.cpp
    Source/Renderer/Vertex.h
    Source/stb_image.h
    Source/stb_image.cpp
    Source/Components.h
    Source/Math.h
    Source/Components.cpp
    Source/Input.h
    Source/Utils.h
    Source/Utils.cpp
    Source/Logger.cpp
    Source/Logger.h
    Source/LoggerMacros.h 
    Source/Physics.cpp
    Source/Physics.h
    Source/LasLoader.cpp 
    Source/LasLoader.h 
    Source/Octree.h 
    Source/Octree.cpp
    Source/Simulate.cpp
    Source/Simulate.h "Source/Renderer/MeshManager.h" "Source/Renderer/TextureManager.h" "Source/Renderer/MeshManager.cpp" "Source/Renderer/TextureManager.cpp" "Source/Renderer/Mesh.h" "Source/Renderer/Texture.h" "Source/Renderer/Texture.cpp" "Source/Renderer/Mesh.cpp")


find_package(Vulkan REQUIRED)
target_include_directories(Floof PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(Floof ${Vulkan_LIBRARIES})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(Libs/glfw)
target_link_libraries(Floof glfw)

add_subdirectory(Libs/entt)
target_link_libraries(Floof EnTT)

#find_package(Bullet REQUIRED)
add_subdirectory(Libs/bullet3)
include_directories(Libs/bullet3/src)
target_link_libraries(Floof BULLET_PHYSICS)

set(IMGUI_DIR Libs/imgui)
add_library(IMGUI STATIC)

target_sources(IMGUI
    PRIVATE
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui.cpp

    PRIVATE
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    )

target_include_directories(IMGUI
    PUBLIC ${IMGUI_DIR}
    PUBLIC ${IMGUI_DIR}/backends
    PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC Libs/glfw/include
    )

target_link_libraries(Floof IMGUI)

target_include_directories(Floof PUBLIC "Libs/HeaderOnly")

exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Basic.vert -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Basic.vert.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Basic.frag -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Basic.frag.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Color.vert -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Color.vert.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Color.frag -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Color.frag.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Normal.vert -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Normal.vert.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Normal.frag -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Normal.frag.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/LitColor.vert -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/LitColor.vert.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/LitColor.frag -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/LitColor.frag.spv")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND 
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
    ${CMAKE_CURRENT_BINARY_DIR}/Shaders
    )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Assets
    ${CMAKE_CURRENT_BINARY_DIR}/Assets
    )