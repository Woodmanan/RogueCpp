﻿# CMakeList.txt : CMake project for RoguelikeFramework, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("RoguelikeFramework" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS ON)


add_library(BearLibTerminal SHARED IMPORTED)
set_property(TARGET BearLibTerminal PROPERTY
             IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}\\code\\libraries\\BearLibTerminal\\Windows64\\BearLibTerminal.dll)
set_property(TARGET BearLibTerminal PROPERTY
             IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}\\code\\libraries\\BearLibTerminal\\Windows64\\BearLibTerminal.lib)

option(TRACY_ENABLE "" ON)
#option(TRACY_ON_DEMAND "" ON)
add_subdirectory(code/libraries/tracy)

# Add source to this project's executable.
file(GLOB_RECURSE RoguelikeFramework_SRC
     "${PROJECT_SOURCE_DIR}/code/source/*.cpp"
     "${PROJECT_SOURCE_DIR}/code/source/*.h"
     "${PROJECT_SOURCE_DIR}/code/libraries/BearLibTerminal/*.h"
)

add_compile_definitions("DEBUG=$<CONFIG:Debug>")
add_compile_definitions("REL_WITH_DEBUG=$<CONFIG:RelWithDebInfo>")
add_executable (RoguelikeFramework ${RoguelikeFramework_SRC})

target_link_libraries(RoguelikeFramework PUBLIC BearLibTerminal)
target_link_libraries(RoguelikeFramework PUBLIC Tracy::TracyClient)

#set_property(TARGET RoguelikeFramework PROPERTY CXX_STANDARD "20")
#set_property(TARGET RoguelikeFramework PROPERTY CXX_STANDARD_REQUIRED TRUE)


# TODO: Add tests and install targets if needed.

#Copy BearLibTerminal.dll for builds
add_custom_command(TARGET RoguelikeFramework POST_BUILD      #Add a post build event
	COMMAND ${CMAKE_COMMAND} -E copy_if_different        #Executes a copy command
	"${PROJECT_SOURCE_DIR}/code/libraries/BearLibTerminal/Windows64/BearLibTerminal.dll"
	$<TARGET_FILE_DIR:RoguelikeFramework>)


#Copy Resources folder
add_custom_command(TARGET RoguelikeFramework POST_BUILD      #Add a post build event
	COMMAND ${CMAKE_COMMAND} -E copy_directory        #Executes a copy command
	"${PROJECT_SOURCE_DIR}/resources"
	"$<TARGET_FILE_DIR:RoguelikeFramework>/resources")
