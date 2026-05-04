cmake_minimum_required(VERSION 3.20)
project(Sim_3D LANGUAGES C CXX)

# --- 1. C++ Standard und MSVC-Fixes ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor)
endif()

# --- 2. Pfad-Definitionen ---
set(EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(BGFX_DIR "${EXT_DIR}/bgfx/bgfx")
set(DEAR_IMGUI_DIR "${EXT_DIR}/dear-imgui")
set(JOLT_DIR "${EXT_DIR}/jolt")

# --- 3. Subdirectories ---
add_subdirectory("${EXT_DIR}/SDL" EXCLUDE_FROM_ALL)
add_subdirectory("${EXT_DIR}/bgfx" EXCLUDE_FROM_ALL)

if(EXISTS "${JOLT_DIR}/Build/CMakeLists.txt")
    add_subdirectory("${JOLT_DIR}/Build" EXCLUDE_FROM_ALL)
endif()

# --- 4. Projekt Sourcen ---
set(PROJECT_SOURCES
    "src/main.cpp"
    "src/imgui_impl_bgfx.cpp" 
    "src/imgui_impl_bgfx.h"
    # Dear ImGui Core
    "${DEAR_IMGUI_DIR}/imgui.cpp"
    "${DEAR_IMGUI_DIR}/imgui_draw.cpp"
    "${DEAR_IMGUI_DIR}/imgui_widgets.cpp"
    "${DEAR_IMGUI_DIR}/imgui_tables.cpp"
    "${DEAR_IMGUI_DIR}/imgui_demo.cpp"
    # Backends
    "${DEAR_IMGUI_DIR}/backends/imgui_impl_sdl3.cpp"
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# --- 5. Include Verzeichnisse ---
target_include_directories(${PROJECT_NAME} PRIVATE 
    BEFORE "${DEAR_IMGUI_DIR}" 
    "${DEAR_IMGUI_DIR}/backends"
    "${BGFX_DIR}/include" 
    "${EXT_DIR}/bgfx/bx/include" 
    "${EXT_DIR}/bgfx/bimg/include"
    "${EXT_DIR}/glm"
    "${JOLT_DIR}"
    "src"
    # HIER IST DIE MAGIE: Der Pfad zu deinen fertigen Shader-Headern!
    "${BGFX_DIR}/examples/common/imgui"
)

# --- 6. Linking ---
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3-shared 
    bgfx 
    bx      
    bimg    
    Jolt
)

# --- 7. Post-Build ---
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        $<TARGET_FILE:SDL3-shared> 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()