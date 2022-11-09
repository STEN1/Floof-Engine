cmake_minimum_required (VERSION 3.14)
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
    Source/Components.h
    Source/Math.h
    Source/Components.cpp
    Source/Input.h
    Source/Utils.h
    Source/Utils.cpp
    Source/LasLoader.cpp
    Source/LasLoader.h
    Source/Renderer/ModelManager.h
    Source/Renderer/TextureManager.h 
    Source/Renderer/ModelManager.cpp 
    Source/Renderer/TextureManager.cpp 
    Source/Renderer/Mesh.h 
    Source/Renderer/Texture.h
    Source/Renderer/SceneRenderer.h 
    Source/Renderer/ForwardSceneRenderer.h 
    Source/Renderer/DeferredSceneRenderer.h 
    Source/Renderer/DeferredSceneRenderer.cpp 
    Source/Renderer/ForwardSceneRenderer.cpp
    Source/Renderer/stb_image.cpp
    Source/Scene.h
    Source/Scene.cpp 
    Source/GameMode/GameMode.h 
    Source/GameMode/PhysicsGM.h 
    Source/GameMode/PhysicsGM.cpp 
    Source/GameMode/SponzaGM.h
    Source/GameMode/SponzaGM.cpp
    Source/PhysicsSystem.cpp
    Source/PhysicsSystem.h
    Source/GameMode/SponzaGM.cpp
    Source/Renderer/Material.h
    Source/GameMode/AudioTestGM.h
    Source/GameMode/AudioTestGM.cpp
    Source/Renderer/Mesh.cpp
    Source/SoundManager.h
    Source/SoundManager.cpp
    Source/Renderer/VulkanBuffer.h
    Source/Renderer/VulkanBuffer.cpp
    Source/NativeScripts/NativeScript.h
    Source/NativeScripts/TestScript.h
    Source/NativeScripts/TestScript.cpp
    Source/Renderer/VulkanBuffer.h
    Source/Renderer/VulkanBuffer.cpp
    Source/Scripts.cpp
    Source/HeightmapLoader.h
    Source/HeightmapLoader.cpp)

find_package(Vulkan REQUIRED)
target_include_directories(Floof PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(Floof ${Vulkan_LIBRARIES})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(Libs/glfw)
target_link_libraries(Floof glfw)

set(BUILD_SHARED_LIBS OFF)
set(BULLET_VERSION 3)
set(BULLET_PHYSICS_SOURCE_DIR ../../)
set(Bullet_Dir Libs/bullet3)
add_library(Bullet STATIC)
#add_compile_definitions(BT_USE_SSE_IN_API) #enable SSE
add_compile_definitions(__BT_DISABLE_SSE__)
target_sources(Bullet PRIVATE
        ${Bullet_Dir}/src/btBulletDynamicsAll.cpp
        ${Bullet_Dir}/src/btBulletCollisionAll.cpp
        ${Bullet_Dir}/src/btLinearMathAll.cpp)
target_include_directories(Floof PUBLIC ${Bullet_Dir})
target_link_libraries(Floof Bullet)

#Add each submodule manually
add_subdirectory(${Bullet_Dir}/src/BulletSoftBody)
target_link_libraries(Floof BulletSoftBody)

#pybinding
find_package(Python COMPONENTS Interpreter Development REQUIRED)
target_include_directories(Floof PUBLIC ${Python_INCLUDE_DIRS})
target_link_libraries(Floof ${Python_LIBRARIES})

target_compile_definitions(Floof PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})

set(LIBTYPE STATIC)
add_compile_definitions(AL_LIBTYPE_STATIC)
add_subdirectory(Libs/openal-soft)
target_link_libraries(Floof OpenAL)

add_subdirectory(Libs/entt)
target_link_libraries(Floof EnTT)

set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
add_subdirectory(Libs/assimp)
target_link_libraries(Floof assimp)

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

#if compiled with xcode, not supporting release yet
if(CMAKE_GENERATOR STREQUAL Xcode)
    set(CMAKE_CURRENT_BINARY_DIR  "Debug/")
endif()

exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Color.vert -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Color.vert.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Color.frag -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Color.frag.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ForwardLit.vert -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ForwardLit.vert.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ForwardLit.frag -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ForwardLit.frag.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Wireframe.vert -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Wireframe.vert.spv")
exec_program("glslc ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Wireframe.frag -o ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Wireframe.frag.spv")


execute_process(
        #OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Source/setup.py install
)

#set current binary dir c++ variable
add_compile_definitions(BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
add_compile_definitions(SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

#copy shaders
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND 
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
    ${CMAKE_CURRENT_BINARY_DIR}/Shaders
    )

#copy assets
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Assets
    ${CMAKE_CURRENT_BINARY_DIR}/Assets
    )

#copy scripts
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Scripts
        ${CMAKE_CURRENT_BINARY_DIR}/Scripts
        )

#check for imgui file if not set default
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
else()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/ImguiDefault/imgui.ini
            ${CMAKE_CURRENT_BINARY_DIR}/imgui.ini
            )
endif()
