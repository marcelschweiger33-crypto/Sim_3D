cmake_minimum_required(VERSION 3.20)
project(Sim_3D)

# C++ Standard und MSVC Fixes
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor)
endif()

# Pfad-Definitionen
set(EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(BGFX_DIR "${EXT_DIR}/bgfx/bgfx")
set(DEAR_IMGUI_DIR "${EXT_DIR}/dear-imgui")

# --- Subdirectories (Externe Abhängigkeiten) ---
add_subdirectory("${EXT_DIR}/SDL" EXCLUDE_FROM_ALL)
add_subdirectory("${EXT_DIR}/bgfx" EXCLUDE_FROM_ALL)

# --- Projekt Sourcen ---
# Beinhaltet Hauptprogramm und die benötigten ImGui-Implementierungen
set(PROJECT_SOURCES
    "src/main.cpp"
    "${DEAR_IMGUI_DIR}/imgui.cpp"
    "${DEAR_IMGUI_DIR}/imgui_draw.cpp"
    "${DEAR_IMGUI_DIR}/imgui_widgets.cpp"
    "${DEAR_IMGUI_DIR}/imgui_tables.cpp"
    "${DEAR_IMGUI_DIR}/backends/imgui_impl_sdl3.cpp"
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# --- Include Verzeichnisse ---
target_include_directories(${PROJECT_NAME} PRIVATE 
    BEFORE "${DEAR_IMGUI_DIR}" 
    "${DEAR_IMGUI_DIR}/backends"
    "${BGFX_DIR}/include" 
    "${EXT_DIR}/bgfx/bx/include" 
    "${EXT_DIR}/bgfx/bimg/include"
    "src"
)

# --- Linking ---
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3-shared 
    bgfx
)

# --- Post-Build: SDL3.dll automatisch in den Build-Ordner kopieren ---
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        $<TARGET_FILE:SDL3-shared> 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()