cmake_minimum_required(VERSION 3.10)
project(Aerial)
set(CMAKE_CXX_STANDARD 20)

# Set paths
set(TARGET_NAME AerialMain)
set(BIN_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
set(RES_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/res)
set(RES_DST_PATH ${BIN_PATH}/res)
set(ENGINE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/engine)
set(DEPS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(TOOLS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tools)

set(GLFW_PATH ${DEPS_PATH}/glfw)
set(IMGUI_PATH ${DEPS_PATH}/imgui)
set(STB_PATH ${DEPS_PATH}/stb)
set(TRACY_PATH ${DEPS_PATH}/tracy)
set(OPENUSD_PATH ${DEPS_PATH}/OpenUSD)
set(OPENUSDINSTALL_PATH ${DEPS_PATH}/install/OpenUSD)
set(LUA_PATH ${DEPS_PATH}/lua)
set(SOL2_PATH ${DEPS_PATH}/sol2)
set(VOLK_PATH ${DEPS_PATH}/volk)

# Main executable
file(GLOB_RECURSE SOURCES
    ${ENGINE_PATH}/*.cpp
    ${ENGINE_PATH}/*.h
    ${ENGINE_PATH}/*.hpp
    ${ENGINE_PATH}/*.c
)
add_executable(${TARGET_NAME} ${SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE ${ENGINE_PATH})
target_precompile_headers(${TARGET_NAME} PRIVATE ${ENGINE_PATH}/aerial/pch.h)

# Defines
target_compile_definitions(${TARGET_NAME} PRIVATE LOGGER_ENABLE)
target_compile_definitions(${TARGET_NAME} PRIVATE DEBUG_MENU_ENABLE)
target_compile_definitions(${TARGET_NAME} PRIVATE TRACY_ENABLE)

# IMGUI
set(IMGUI_SOURCES
        ${IMGUI_PATH}/imgui/imgui.cpp
        ${IMGUI_PATH}/imgui/imgui_draw.cpp
        ${IMGUI_PATH}/imgui/imgui_widgets.cpp
        ${IMGUI_PATH}/imgui/imgui_demo.cpp
        ${IMGUI_PATH}/imgui/imgui_tables.cpp
        ${IMGUI_PATH}/imgui/backends/imgui_impl_glfw.cpp
        ${IMGUI_PATH}/imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(${TARGET_NAME} PRIVATE ${IMGUI_PATH})
target_sources(${TARGET_NAME} PRIVATE ${IMGUI_SOURCES})

# GLFW
add_subdirectory(${GLFW_PATH})
target_link_libraries(${TARGET_NAME} PRIVATE glfw)
target_include_directories(${TARGET_NAME} PRIVATE ${GLFW_PATH}/include)

# STB
target_include_directories(${TARGET_NAME} PRIVATE ${STB_PATH})

# TRACY
target_include_directories(${TARGET_NAME} PRIVATE ${TRACY_PATH}/public)
target_sources(${TARGET_NAME} PRIVATE ${TRACY_PATH}/public/TracyClient.cpp)
set_source_files_properties(${TRACY_PATH}/public/TracyClient.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# OpenUSD
if(WIN32)
    execute_process(COMMAND cmd /c ${TOOLS_PATH}/buildOpenUSD.bat)
endif()
target_include_directories(${TARGET_NAME} SYSTEM  PRIVATE "${OPENUSDINSTALL_PATH}/include")
file(GLOB OPENUSD_LIBS "${OPENUSDINSTALL_PATH}/lib/*.lib")
foreach(LIB ${OPENUSD_LIBS})
    target_link_libraries(${TARGET_NAME} PRIVATE ${LIB})
endforeach()

# Lua
file(GLOB_RECURSE LUA_SOURCES
        ${LUA_PATH}/l*.h
        ${LUA_PATH}/l*.c
)
add_library(lua ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${LUA_PATH})
target_link_libraries(${TARGET_NAME} PRIVATE lua)

# Sol2
set(SOL2_LUA_VERSION "5.4.6" CACHE STRING "The version of Lua used")
set(SOL2_BUILD_LUA FALSE CACHE BOOL "Always build Lua, do not search for it in the system")
add_subdirectory(${SOL2_PATH})
target_link_libraries(${TARGET_NAME} PRIVATE sol2)

# Volk
set (GLSLANG_VALIDATOR "glslangValidator")
if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()
add_subdirectory(${VOLK_PATH})
target_link_libraries(${TARGET_NAME} PRIVATE volk)

# Post-build
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${RES_DST_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_SRC_PATH} ${RES_DST_PATH}
)

if(WIN32)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${OPENUSDINSTALL_PATH}/lib" ${BIN_PATH}
    )

    file(GLOB DLL_FILES "${OPENUSDINSTALL_PATH}/bin/*")
    foreach(DLL_FILE ${DLL_FILES})
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${DLL_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
        )
    endforeach()
endif()
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${OPENUSDINSTALL_PATH}/plugin/usd ${BIN_PATH}/usd
)

set(SHADERS_SRC_DIR ${RES_DST_PATH}/shaders)
set(SHADERS_DST_DIR ${RES_DST_PATH}/shaders_compiled)
add_custom_command(
    TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DST_DIR}
    COMMAND ${GLSLANG_VALIDATOR} -V -o ${SHADERS_DST_DIR}/vert.spv ${SHADERS_SRC_DIR}/shader.vert
    DEPENDS shader.vert ${GLSLANG_VALIDATOR}
    COMMAND ${GLSLANG_VALIDATOR} -V -o ${SHADERS_DST_DIR}/frag.spv ${SHADERS_SRC_DIR}/shader.frag
    DEPENDS shader.frag ${GLSLANG_VALIDATOR}
)

set(ENV{PIXAR_TF_ENV_SETTING_FILE} ${RES_DST_PATH}/usd/env-settings)

