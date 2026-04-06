cmake_minimum_required(VERSION 3.1...4.0)
project(RogueCpp LANGUAGES CXX)

# ============================================================
# C++ configuration
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

# ============================================================
# Determine Architecture
# ============================================================
set(BEARLIBTERMINAL_FORCE_BITS "auto"
    CACHE STRING "BearLibTerminal architecture: auto, 32, or 64"
)
set_property(CACHE BEARLIBTERMINAL_FORCE_BITS PROPERTY STRINGS auto 32 64)

if(BEARLIBTERMINAL_FORCE_BITS STREQUAL "auto")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(BEARLIBTERMINAL_ARCH_BITS 64)
    else()
        set(BEARLIBTERMINAL_ARCH_BITS 32)
    endif()
else()
    set(BEARLIBTERMINAL_ARCH_BITS ${BEARLIBTERMINAL_FORCE_BITS})
endif()

# ============================================================
# Source files
# ============================================================
file(GLOB_RECURSE ROGUECPP_SOURCES
    CONFIGURE_DEPENDS
    code/source/*.cpp
)

if(ROGUECPP_SOURCES STREQUAL "")
    message(FATAL_ERROR "No source files found in code/source/")
endif()

add_executable(${PROJECT_NAME} ${ROGUECPP_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/source)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:DEBUG_FULL>
    $<$<CONFIG:RelWithDebInfo>:DEBUG>
    $<$<CONFIG:Release>:RELEASE>
)
 

# A cross-platform way to add flags if needed:
if(MSVC)
    # No extra flag usually needed for MSVC
elseif(UNIX)
    target_compile_options(${PROJECT_NAME} PRIVATE "-fms-extensions")
endif()

# clang template debugging
target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:
            -fdiagnostics-show-template-tree
            -fdiagnostics-show-note-include-stack
            -ftemplate-backtrace-limit=10
	    -ferror-limit=1
        >
)


# ============================================================
# Dependencies
# ============================================================
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# ============================================================
# Vulkan - Not downloaded, required install
# ============================================================
find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Vulkan::Vulkan
)

# ============================================================
# GLFW
# ============================================================
# Disable unnecessary builds
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

# Force X11, disable Wayland
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)

# Disable OpenGL and GLES
set(GLFW_USE_OPENGL OFF CACHE BOOL "" FORCE)
set(GLFW_INCLUDE_GLCOREARB OFF CACHE BOOL "" FORCE)
set(GLFW_USE_GLES OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# ------------------------------------------------------------
# magic_enum (header-only)
# ------------------------------------------------------------
FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(magic_enum)
target_link_libraries(${PROJECT_NAME} PRIVATE magic_enum::magic_enum)

# ------------------------------------------------------------
# GLM
# ------------------------------------------------------------
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
)

FetchContent_MakeAvailable(glm)

target_link_libraries(${PROJECT_NAME}
    PRIVATE glm::glm
)

# ------------------------------------------------------------
# Tracy (profiling)
# ------------------------------------------------------------
option(TRACY_ENABLE "" ON)
option(TRACY_ON_DEMAND "" OFF)

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.12.0
)
FetchContent_MakeAvailable(tracy)
target_link_libraries(${PROJECT_NAME} PRIVATE Tracy::TracyClient)
target_compile_definitions(${PROJECT_NAME} PRIVATE TRACY_ENABLE)

# ------------------------------------------------------------
# Freetype
# ------------------------------------------------------------
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype
    GIT_TAG VER-2-14-1
)

FetchContent_MakeAvailable(freetype)
target_link_libraries(${PROJECT_NAME}
    PRIVATE freetype
)

# ------------------------------------------------------------
# RectPack2D 
# ------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    rectpack2D
    GIT_REPOSITORY https://github.com/TeamHypersomnia/rectpack2D.git
    GIT_TAG master
)

FetchContent_MakeAvailable(rectpack2D)

target_include_directories(rectpack2D INTERFACE
    ${rectpack2D_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE rectpack2D::rectpack2D
)

# ------------------------------------------------------------
# stb
# ------------------------------------------------------------
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)

FetchContent_MakeAvailable(stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
    ${stb_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE stb
)

# ------------------------------------------------------------
# Catch2
# ------------------------------------------------------------
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.13.0
)

FetchContent_MakeAvailable(Catch2)
target_link_libraries(${PROJECT_NAME}
    PRIVATE Catch2
)

# ------------------------------------------------------------
# CLI 11 - CLI Utils 
# ------------------------------------------------------------
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    GIT_TAG v2.6.2
)

FetchContent_MakeAvailable(CLI11)
target_link_libraries(${PROJECT_NAME}
    PRIVATE CLI11
)

# ============================================================
# BearLibTerminal (prebuilt binaries)
# ============================================================
set(BEARLIBTERMINAL_VERSION 0.15.8)

if(WIN32)
    set(BEARLIBTERMINAL_ARCHIVE bearlibterminal_${BEARLIBTERMINAL_VERSION}.zip)
    set(BEARLIBTERMINAL_LIBRARY BearLibTerminal.dll)
    set(BEARLIBTERMINAL_URL http://foo.wyrd.name/_media/en:bearlibterminal:bearlibterminal_0.15.8.zip)
elseif(APPLE)
    set(BEARLIBTERMINAL_ARCHIVE bearlibterminal_${BEARLIBTERMINAL_VERSION}_osx.zip)
    set(BEARLIBTERMINAL_LIBRARY libBearLibTerminal.dylib)
    set(BEARLIBTERMINAL_URL http://foo.wyrd.name/_media/en:bearlibterminal:bearlibterminal_0.15.8_osx.zip)
elseif(UNIX)
    set(BEARLIBTERMINAL_ARCHIVE bearlibterminal_${BEARLIBTERMINAL_VERSION}.tar.bz2)
    set(BEARLIBTERMINAL_LIBRARY libBearLibTerminal.so)
    set(BEARLIBTERMINAL_URL http://foo.wyrd.name/_media/en:bearlibterminal:bearlibterminal_0.15.8.tar.bz2)
else()
    message(FATAL_ERROR "Unsupported platform for BearLibTerminal")
endif()

set(BEARLIBTERMINAL_DIR ${CMAKE_BINARY_DIR}/bearlibterminal)
set(BEARLIBTERMINAL_ARCHIVE_PATH ${BEARLIBTERMINAL_DIR}/${BEARLIBTERMINAL_ARCHIVE})

# Only download if the archive does not exist
if(NOT EXISTS ${BEARLIBTERMINAL_ARCHIVE_PATH})
    file(MAKE_DIRECTORY ${BEARLIBTERMINAL_DIR})
    message(STATUS "Downloading BearLibTerminal from ${BEARLIBTERMINAL_URL}")
    file(DOWNLOAD
        ${BEARLIBTERMINAL_URL}
        ${BEARLIBTERMINAL_ARCHIVE_PATH}
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
    if(NOT DOWNLOAD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download BearLibTerminal")
    endif()
else()
    message(STATUS "BearLibTerminal archive already exists, skipping download")
endif()

# Only extract if the library does not exist
file(GLOB_RECURSE BEARLIBTERMINAL_LIB_PATH
    ${BEARLIBTERMINAL_DIR}/*/${BEARLIBTERMINAL_LIBRARY}
)

if(BEARLIBTERMINAL_LIB_PATH STREQUAL "")
    message(STATUS "Extracting BearLibTerminal archive")
    if(WIN32 OR APPLE)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${BEARLIBTERMINAL_ARCHIVE_PATH}
            WORKING_DIRECTORY ${BEARLIBTERMINAL_DIR}
        )
    elseif(UNIX)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xjf ${BEARLIBTERMINAL_ARCHIVE_PATH}
            WORKING_DIRECTORY ${BEARLIBTERMINAL_DIR}
        )
    endif()

    file(GLOB_RECURSE BEARLIBTERMINAL_LIB_PATH
        ${BEARLIBTERMINAL_DIR}/*/${BEARLIBTERMINAL_LIBRARY}
    )

    if(BEARLIBTERMINAL_LIB_PATH STREQUAL "")
        message(FATAL_ERROR "BearLibTerminal library not found after extraction")
    endif()
else()
    message(STATUS "BearLibTerminal library already extracted, skipping extraction")
endif()

list(LENGTH BEARLIBTERMINAL_LIB_PATH _lib_count)
if(_lib_count GREATER 1)
    file(GLOB_RECURSE _BEARLIBTERMINAL_CANDIDATES
        ${BEARLIBTERMINAL_DIR}/*/${BEARLIBTERMINAL_LIBRARY}
    )

    set(BEARLIBTERMINAL_LIB_PATH "")

    foreach(_lib ${_BEARLIBTERMINAL_CANDIDATES})
        if(_lib MATCHES "64" AND BEARLIBTERMINAL_ARCH_BITS EQUAL 64)
            set(BEARLIBTERMINAL_LIB_PATH "${_lib}")
            break()
        elseif(_lib MATCHES "32" AND BEARLIBTERMINAL_ARCH_BITS EQUAL 32)
            set(BEARLIBTERMINAL_LIB_PATH "${_lib}")
            break()
        endif()
    endforeach()
    
    if(BEARLIBTERMINAL_LIB_PATH STREQUAL "")
        message(FATAL_ERROR "No BearLibTerminal ${BEARLIBTERMINAL_ARCH_BITS}-bit library found")
    endif()
elseif(_lib_count EQUAL 1)
    list(GET BEARLIBTERMINAL_LIB_PATH 0 BEARLIBTERMINAL_LIB_PATH)
else()
	message(FATAL_ERROR "No BearLibTerminal library could be found.")
endif()

#Find the libary path dynamically
set(BEARLIBTERMINAL_INCLUDE_DIR ${BEARLIBTERMINAL_DIR}/BearLibTerminal_${BEARLIBTERMINAL_VERSION}/Include)

message(STATUS "BearLibTerminal lib path set to ${BEARLIBTERMINAL_LIB_PATH}")
message(STATUS "BearLibTerminal include dir set to ${BEARLIBTERMINAL_INCLUDE_DIR}")

add_library(BearLibTerminal SHARED IMPORTED)

set_target_properties(BearLibTerminal PROPERTIES
    IMPORTED_LOCATION ${BEARLIBTERMINAL_LIB_PATH}
    INTERFACE_INCLUDE_DIRECTORIES ${BEARLIBTERMINAL_INCLUDE_DIR} 
)

# Linux system dependencies required by BearLibTerminal
if(UNIX AND NOT APPLE)
    target_link_libraries(BearLibTerminal INTERFACE dl pthread X11)
endif()

# Link to RogueCpp
target_link_libraries(${PROJECT_NAME} PRIVATE BearLibTerminal)

# Copy runtime library next to executable (standalone)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${BEARLIBTERMINAL_LIB_PATH}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# ============================================================
# Compiler warnings
# ============================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

