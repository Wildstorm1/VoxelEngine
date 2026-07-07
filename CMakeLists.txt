cmake_minimum_required(VERSION 3.10)
project(PrestoEngine)

add_custom_target(CopyShaders COMMAND cp -r "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/" "${CMAKE_BINARY_DIR}")

add_custom_target(CompileShaders COMMAND cmd /c "${CMAKE_CURRENT_SOURCE_DIR}/shaders.bat")
add_dependencies(CompileShaders CopyShaders)

set(CMAKE_CXX_STANDARD 26)
find_package(Vulkan REQUIRED)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

add_dependencies(${PROJECT_NAME} CompileShaders)