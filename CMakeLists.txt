cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME pefti)
set(INCLUDE_DIR include)
set(SOURCE_DIR src)
set(SOURCE_FILES
    ${SOURCE_DIR}/application.cc
    ${SOURCE_DIR}/config.cc
    ${SOURCE_DIR}/epg.cc
    ${SOURCE_DIR}/filter.cc
    ${SOURCE_DIR}/iptv_channel.cc
    ${SOURCE_DIR}/loader.cc
    ${SOURCE_DIR}/main.cc
    ${SOURCE_DIR}/mapper.cc
    ${SOURCE_DIR}/parser.cc
    ${SOURCE_DIR}/playlist.cc
    ${SOURCE_DIR}/resource.cc
    ${SOURCE_DIR}/sax_fsm.cc
    ${SOURCE_DIR}/toml_config_reader.cc
    ${SOURCE_DIR}/transformer.cc
)

project(${PROJECT_NAME} LANGUAGES CXX)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # -Wno-array-bounds to suppress toml++ warnings
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-array-bounds)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # TODO: add macOS-specific flags 
endif()

# External Packages

find_package(LibXml2 REQUIRED)
find_package(OpenSSL REQUIRED)

include(FetchContent)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0)
FetchContent_MakeAvailable(tomlplusplus)
include_directories(${tomlplusplus_SOURCE_DIR}/include)

FetchContent_Declare(curl
    URL https://github.com/curl/curl/releases/download/curl-8_5_0/curl-8.5.0.zip)
FetchContent_MakeAvailable(curl)
include_directories(${curl_SOURCE_DIR}/include)

FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.1.1)
FetchContent_MakeAvailable(cxxopts)
include_directories(${cxxopts_SOURCE_DIR}/include)

FetchContent_Declare(GSL
    GIT_REPOSITORY "https://github.com/microsoft/GSL"
    GIT_TAG "v4.0.0"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(GSL)

FetchContent_Declare(
    cppcoro
    GIT_REPOSITORY https://github.com/andreasbuhr/cppcoro.git
    GIT_TAG 244f834274a850eeca4cc5c86acbca1a0fbe9d95)
set(BUILD_TESTING OFF)
FetchContent_MakeAvailable(cppcoro)
unset(BUILD_TESTING)
include_directories(${cppcoro_SOURCE_DIR}/include)

target_include_directories (${PROJECT_NAME} PRIVATE ${LIBXML2_INCLUDE_DIR})
target_link_libraries (${PROJECT_NAME} PRIVATE ${LIBXML2_LIBRARIES} libcurl)
target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft.GSL::GSL)
target_link_libraries (${PROJECT_NAME} PRIVATE cppcoro)

