# CMakeList.txt : CMake project for RoguelikeFramework, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("RoguelikeFramework" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_COMPILER_WORKS 1)

find_package(Vulkan REQUIRED FATAL_ERROR)

add_library(glfw SHARED IMPORTED)
set_property(TARGET glfw PROPERTY
             IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}\\code\\libraries\\glfw-3.4.bin.WIN64\\lib-vc2019\\glfw3.dll)
set_property(TARGET glfw PROPERTY
             IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}\\code\\libraries\\glfw-3.4.bin.WIN64\\lib-vc2019\\glfw3.lib)

add_library(BearLibTerminal SHARED IMPORTED)
set_property(TARGET BearLibTerminal PROPERTY
             IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}\\code\\libraries\\BearLibTerminal\\Windows64\\BearLibTerminal.dll)
set_property(TARGET BearLibTerminal PROPERTY
             IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}\\code\\libraries\\BearLibTerminal\\Windows64\\BearLibTerminal.lib)

option(TRACY_ENABLE "" ON)
option(TRACY_ON_DEMAND "" ON)
add_subdirectory(code/libraries/tracy)

add_subdirectory(code/libraries/magic_enum)

# Add source to this project's executable.
file(GLOB_RECURSE RoguelikeFramework_SRC
     "${PROJECT_SOURCE_DIR}/code/source/*.cpp"
     "${PROJECT_SOURCE_DIR}/code/source/*.h"
	 "${PROJECT_SOURCE_DIR}/libraries/*.hpp"
	 "${PROJECT_SOURCE_DIR}/code/libraries/BearLibTerminal/Include/C/BearLibTerminal.h"
)

add_compile_definitions("DEBUG=$<CONFIG:Debug>")
add_compile_definitions("REL_WITH_DEBUG=$<CONFIG:RelWithDebInfo>")
add_executable (RoguelikeFramework ${RoguelikeFramework_SRC})

target_link_libraries(RoguelikeFramework PUBLIC BearLibTerminal)
target_link_libraries(RoguelikeFramework PUBLIC Tracy::TracyClient)
target_link_libraries(RoguelikeFramework PUBLIC ${Vulkan_LIBRARIES})
target_link_libraries(RoguelikeFramework PUBLIC glfw)
target_link_libraries(RoguelikeFramework PUBLIC magic_enum)
target_link_libraries(RoguelikeFramework PUBLIC "${PROJECT_SOURCE_DIR}/code/libraries/freetype2/release static/vs2015-2022/win64/freetype.lib")

target_include_directories(RoguelikeFramework PUBLIC "C:/VulkanSDK/1.3.290.0/Include")
target_include_directories(RoguelikeFramework PUBLIC "${PROJECT_SOURCE_DIR}/code/libraries/glfw-3.4.bin.WIN64/include")
target_include_directories(RoguelikeFramework PUBLIC "${PROJECT_SOURCE_DIR}/code/source")
target_include_directories(RoguelikeFramework PUBLIC "${PROJECT_SOURCE_DIR}/code/libraries/stb")
target_include_directories(RoguelikeFramework PUBLIC "${PROJECT_SOURCE_DIR}/code/libraries/freetype2/include")
target_include_directories(RoguelikeFramework PUBLIC "${PROJECT_SOURCE_DIR}/code/libraries/rectpack2D")
target_include_directories(RoguelikeFramework PUBLIC "${PROJECT_SOURCE_DIR}/code/libraries/magic_enum")
target_include_directories(RoguelikeFramework PUBLIC "${PROJECT_SOURCE_DIR}/code/libraries/BearLibTerminal")

#set_property(TARGET RoguelikeFramework PROPERTY CXX_STANDARD "20")
#set_property(TARGET RoguelikeFramework PROPERTY CXX_STANDARD_REQUIRED TRUE)


# TODO: Add tests and install targets if needed.

#Copy BearLibTerminal.dll for builds
add_custom_command(TARGET RoguelikeFramework POST_BUILD      #Add a post build event
	COMMAND ${CMAKE_COMMAND} -E copy_if_different        #Executes a copy command
	"${PROJECT_SOURCE_DIR}/code/libraries/BearLibTerminal/Windows64/BearLibTerminal.dll"
	$<TARGET_FILE_DIR:RoguelikeFramework>)

#Copy glfw3.dll for builds
add_custom_command(TARGET RoguelikeFramework POST_BUILD      #Add a post build event
	COMMAND ${CMAKE_COMMAND} -E copy_if_different        #Executes a copy command
	"${PROJECT_SOURCE_DIR}/code/libraries/glfw-3.4.bin.WIN64/lib-vc2019/glfw3.dll"
	$<TARGET_FILE_DIR:RoguelikeFramework>)


#Copy Resources folder
add_custom_command(TARGET RoguelikeFramework POST_BUILD      #Add a post build event
	COMMAND ${CMAKE_COMMAND} -E copy_directory        #Executes a copy command
	"${PROJECT_SOURCE_DIR}/resources"
	"$<TARGET_FILE_DIR:RoguelikeFramework>/resources")
