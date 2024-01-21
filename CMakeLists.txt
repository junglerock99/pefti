cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME pefti)
set(INCLUDE_DIR include)
set(SOURCE_DIR src)
set(SOURCE_FILES
    ${SOURCE_DIR}/application.cc
    ${SOURCE_DIR}/config.cc
    ${SOURCE_DIR}/filter.cc
    ${SOURCE_DIR}/iptv_channel.cc
    ${SOURCE_DIR}/main.cc
    ${SOURCE_DIR}/playlist.cc
    ${SOURCE_DIR}/sorter.cc
    ${SOURCE_DIR}/transformer.cc)

project(${PROJECT_NAME}
    LANGUAGES CXX)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    #target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-array-bounds)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # TODO: add macOS-specific flags 
endif()

include(FetchContent)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)
include_directories(${tomlplusplus_SOURCE_DIR}/include)
