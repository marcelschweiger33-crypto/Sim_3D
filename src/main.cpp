#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui_impl_bgfx.h"
#include <stdio.h>

int main(int argc, char** argv) {
    // 1. SDL Initialisierung
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL konnte nicht initialisiert werden: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Sim_3D - bgfx & ImGui", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Fenster konnte nicht erstellt werden: %s\n", SDL_GetError());
        return -1;
    }

    // 2. bgfx Initialisierung
    bgfx::Init init;
    init.resolution.width = 1280;
    init.resolution.height = 720;
    init.resolution.reset = BGFX_RESET_VSYNC;
    
    // Plattformspezifisches Setup für bgfx an SDL3 binden
    bgfx::PlatformData pd;
#if defined(SDL_PLATFORM_WINDOWS)
    pd.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif
    init.platformData = pd;
    bgfx::init(init);

    // Hintergrundfarbe setzen (Dunkelgrau)
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, 1280, 720);

    // 3. ImGui Initialisierung
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Backends initialisieren
    ImGui_ImplSDL3_InitForOther(window);
    ImGui_Implbgfx_Init(255); // Nutzt View ID 255 für das ImGui-Rendering

    // 4. Main Loop
    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                // Bei Größenänderung des Fensters muss bgfx aktualisiert werden
                bgfx::reset(event.window.data1, event.window.data2, BGFX_RESET_VSYNC);
                bgfx::setViewRect(0, 0, 0, (uint16_t)event.window.data1, (uint16_t)event.window.data2);
            }
        }

        // Start ImGui Frame
        ImGui_Implbgfx_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // --- DEIN GUI CODE HIER ---
        ImGui::Begin("Sim_3D Einstellungen");
        ImGui::Text("Willkommen in deiner 3D-Engine!");
        ImGui::Text("SDL3 + bgfx + ImGui sind erfolgreich verknüpft.");
        ImGui::Spacing();
        if (ImGui::Button("Beenden")) {
            quit = true;
        }
        ImGui::End();
        // --------------------------

        // ImGui Render-Befehle generieren
        ImGui::Render();
        
        // bgfx Frame abschicken
        bgfx::touch(0); // Garantiert, dass der Frame gezeichnet wird, auch wenn nichts passiert
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        bgfx::frame();
    }

    // 5. Cleanup
    ImGui_Implbgfx_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}