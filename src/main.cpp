#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui_impl_bgfx.h"

#include <iostream>

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

void log();

std::string version = "0.1.0";

int main(int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Sim_3D - Vulkan Renderer",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (!window)
    {
        return -1;
    }

    bgfx::Init init;
    init.type = bgfx::RendererType::Vulkan;
    init.resolution.width = WINDOW_WIDTH;
    init.resolution.height = WINDOW_HEIGHT;

    // VSync deaktiviert
    init.resolution.reset = BGFX_RESET_NONE;

    bgfx::PlatformData pd;

#if defined(SDL_PLATFORM_WINDOWS)
    pd.nwh = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER,
        nullptr
    );
#endif

    init.platformData = pd;

    if (!bgfx::init(init))
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Renderer Fehler",
            "Vulkan konnte nicht initialisiert werden!",
            window
        );
        return -1;
    }

    // ImGui Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForOther(window);
    ImGui_Implbgfx_Init(255);

    bool quit = false;
    log();

    while (!quit)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }

            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                uint32_t w = static_cast<uint32_t>(event.window.data1);
                uint32_t h = static_cast<uint32_t>(event.window.data2);

                // Resize ohne VSync
                bgfx::reset(w, h, BGFX_RESET_NONE);
            }
        }

        // Hintergrund setzen
        bgfx::setViewClear(
            0,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            0x151515ff,
            1.0f,
            0
        );

        bgfx::setViewRect(
            0,
            0,
            0,
            static_cast<uint16_t>(ImGui::GetIO().DisplaySize.x),
            static_cast<uint16_t>(ImGui::GetIO().DisplaySize.y)
        );

        bgfx::touch(0);

        // ImGui Frame starten
        ImGui_Implbgfx_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // UI
        ImGui::Begin("Simulator Steuerung");
        ImGui::Text("Renderer: Vulkan");
        ImGui::Text("FPS: %.1f (VSync OFF)", ImGui::GetIO().Framerate);

        if (ImGui::Button("Beenden"))
        {
            quit = true;
        }

        ImGui::End();

        // Rendern
        ImGui::Render();

        bgfx::setViewRect(
            255,
            0,
            0,
            static_cast<uint16_t>(ImGui::GetIO().DisplaySize.x),
            static_cast<uint16_t>(ImGui::GetIO().DisplaySize.y)
        );

        bgfx::setViewClear(255, BGFX_CLEAR_NONE, 0, 1.0f, 0);

        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        bgfx::frame();
    }

    // Cleanup
    ImGui_Implbgfx_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void log()
{
    std::cout << "Sim_3D - Engin Log: " << std::endl;
    std::cout << "Version: "<< version << std::endl;
    std::cout << "------------------------------" << std::endl;

}