cmake_minimum_required(VERSION 3.13)
project(Vulkan_Volumetric_Renderer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
endif(CMAKE_COMPILER_IS_GNUCXX)
if(WIN32)
    add_definitions(-DNOMINMAX)
endif(WIN32)

find_package(Threads REQUIRED)
find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

set(IGG_COMPONENTS "IMGUI;GLFW;GLM" CACHE STRING "The components to import and build")

foreach (comp IN ITEMS ${IGG_COMPONENTS})
    set(IMPORT_${comp} ON)
endforeach ()

if (IMPORT_IMGUI)
    set(IMPORT_GLFW ON)
endif ()

if (IMPORT_IMGUI)
    set(IGG_IMGUI_TAG master CACHE STRING "Defines the imgui version (e.g. master, v1.89.4, v1.89.3, ...")
    set(IMPORT_GLFW ON)
else ()
    unset(IGG_IMGUI_TAG CACHE)
endif ()
if (IMPORT_GLFW)
    set(IGG_GLFW_TAG master CACHE STRING "Defines the glfw version (e.g. master, 3.3.8, 3.3.7, ...) ")
else ()
    unset(IGG_GLFW_TAG CACHE)
endif ()
if (IMPORT_GLM)
    set(IGG_GLM_TAG master CACHE STRING "Defines the glm version (e.g. master, 0.9.9.8, 0.9.9.7, ...")
else ()
    unset(IGG_GLM_TAG CACHE)
endif ()

if (IMPORT_GLFW)
    FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG ${IGG_GLFW_TAG}
    )
    list(APPEND components glfw)
endif ()

if (IMPORT_GLM)
    FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG ${IGG_GLM_TAG}
    )
    list(APPEND components glm)
endif ()

if (IMPORT_IMGUI)
    FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG ${IGG_IMGUI_TAG}
    )
    list(APPEND components imgui)
endif ()

if (components)
    FetchContent_MakeAvailable(${components})
endif ()

if (IMPORT_IMGUI)
    file(GLOB imgui_SRC ${imgui_SOURCE_DIR}/*.cpp ${imgui_SOURCE_DIR}/*.h)
    add_library(
            imgui
            ${imgui_SRC}
            ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.h
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
    )
    target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)
    target_include_directories(
            imgui
            PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )
endif ()


file (GLOB SRCS
	${PROJECT_SOURCE_DIR}/src/*.h
	${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME} ${SRCS})

if(WIN32)
    add_definitions(-DNOMINMAX)
    set(GLSLC glslc.exe)
else()
    set(GLSLC glslc)
endif(WIN32)

if(APPLE)
    add_definitions(-DVK_ENABLE_BETA_EXTENSIONS)
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME_WE)
  get_filename_component(FILE_EXT ${GLSL} LAST_EXT)
  string(REPLACE "." "" FILE_TYPE ${FILE_EXT})
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}_${FILE_TYPE}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSLC} ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

add_dependencies(${PROJECT_NAME} Shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"

    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/textures/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${PROJECT_SOURCE_DIR}/textures"
	"${PROJECT_BINARY_DIR}/textures"
)

target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan glm imgui fmt::fmt)
