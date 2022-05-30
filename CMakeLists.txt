cmake_minimum_required(VERSION 3.19)


project(vulkan_engine LANGUAGES CXX)


set(CLANG_OPTIONS
        -Wfatal-errors
        -Wall
        -Wextra
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        )

set(GCC_OPTIONS
        ${CLANG_OPTIONS}
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
        )

set(MSVC_OPTIONS /W3)

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_OPTIONS ${CLANG_OPTIONS})
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_OPTIONS ${GCC_OPTIONS})
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(PROJECT_OPTIONS ${MSVC_OPTIONS})
else ()
    message(WARNING "No warnings set for compiler '${CMAKE_CXX_COMPILER_ID}'")
endif ()


add_executable(vulkan_engine
        src/main.cpp
        )
target_compile_options(vulkan_engine PRIVATE ${PROJECT_OPTIONS})
target_compile_features(vulkan_engine PRIVATE cxx_std_20)


set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)
target_include_directories(vulkan_engine PRIVATE external/glfw/include)
target_link_libraries(vulkan_engine PRIVATE glfw)


find_package(Vulkan REQUIRED)
target_include_directories(vulkan_engine PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan_engine PRIVATE ${Vulkan_LIBRARIES})
