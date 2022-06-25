cmake_minimum_required(VERSION 3.19)


project(vulkan_engine LANGUAGES CXX)


set(CLANG_OPTIONS
        -ffast-math
        -march=native
        -Wfatal-errors
        -Wall
        -Wextra
        -Wshadow
        -Wnon-virtual-dtor
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
        -Wswitch-enum
        )

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_OPTIONS ${CLANG_OPTIONS})
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_OPTIONS ${GCC_OPTIONS})
else ()
    message(WARNING "No warnings set for compiler '${CMAKE_CXX_COMPILER_ID}'")
endif ()


add_executable(vulkan_engine
        src/main.cpp
        src/application.cpp src/application.hpp
        src/renderer.cpp src/renderer.hpp
        src/utils.cpp src/utils.hpp
        external/stb/stb_image.h
        external/stb/stb_image_write.h
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_vulkan.cpp
        external/imgui/imgui.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        )
target_include_directories(vulkan_engine PRIVATE
        external/stb
        external/imgui
        external/imgui/backends)
target_compile_options(vulkan_engine PRIVATE ${PROJECT_OPTIONS})
target_compile_features(vulkan_engine PRIVATE cxx_std_20)


set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)
target_include_directories(vulkan_engine PRIVATE external/glfw/include)
target_link_libraries(vulkan_engine PRIVATE glfw)


add_subdirectory(external/glm)
target_include_directories(vulkan_engine PRIVATE external/glm)
target_link_libraries(vulkan_engine PRIVATE glm)


find_package(Vulkan REQUIRED)
target_include_directories(vulkan_engine PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan_engine PRIVATE ${Vulkan_LIBRARIES})
message(STATUS "Vulkan SDK: " $ENV{VULKAN_SDK})


set(SHADERS
        shaders/offscreen.vert shaders/offscreen.frag
        shaders/final.vert shaders/final.frag
        )
foreach (SHADER IN LISTS SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
    set(SRC_SHADER ${CMAKE_SOURCE_DIR}/${SHADER})
    set(SPV_SHADER ${CMAKE_SOURCE_DIR}/shaders/spv/${FILENAME}.spv)
    add_custom_command(
            OUTPUT ${SPV_SHADER}
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SRC_SHADER} -o ${SPV_SHADER}
            DEPENDS ${SRC_SHADER}
            COMMENT "Compiling ${SHADER}")
    list(APPEND SPV_SHADERS ${SPV_SHADER})
endforeach ()
add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})
add_dependencies(vulkan_engine shaders)
