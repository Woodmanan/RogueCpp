# CMakeList.txt : CMake project for RoguelikeFramework, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)

project ("RoguelikeFramework")

add_library(BearLibTerminal SHARED IMPORTED)
set_property(TARGET BearLibTerminal PROPERTY
             IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}\\code\\libraries\\BearLibTerminal\\Windows64\\BearLibTerminal.dll)
set_property(TARGET BearLibTerminal PROPERTY
             IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}\\code\\libraries\\BearLibTerminal\\Windows64\\BearLibTerminal.lib)

# Add source to this project's executable.
file(GLOB_RECURSE RoguelikeFramework_SRC
     "${PROJECT_SOURCE_DIR}/code/source/*.cpp"
     "${PROJECT_SOURCE_DIR}/code/source/*.h"
     "${PROJECT_SOURCE_DIR}/code/libraries/BearLibTerminal/*.h"
)
add_executable (RoguelikeFramework ${RoguelikeFramework_SRC})

target_link_libraries(RoguelikeFramework PUBLIC BearLibTerminal)

set_property(TARGET RoguelikeFramework PROPERTY CXX_STANDARD "20")
set_property(TARGET RoguelikeFramework PROPERTY CXX_STANDARD_REQUIRED TRUE)


# TODO: Add tests and install targets if needed.

#Install Libraries?
#install(BearLibTerminal RUNTIME_DEPENDENCY_SET)