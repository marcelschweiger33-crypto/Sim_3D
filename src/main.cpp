#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui_impl_bgfx.h"
#include <stdio.h>

int main(int argc, char** argv) {
    // 1. Variablen (Zuvor deklarieren)
    float myCpuValue = 12.5f;
    int usedVram = 1024;
    int totalVram = 8192;

    // 2. SDL Initialisierung
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Sim_3D", 1280, 720, SDL_WINDOW_RESIZABLE);

    // 3. bgfx Initialisierung
    bgfx::Init init;
    init.resolution.width = 1280;
    init.resolution.height = 720;
    init.resolution.reset = BGFX_RESET_VSYNC;
    
    bgfx::PlatformData pd;
#if defined(SDL_PLATFORM_WINDOWS)
    pd.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif
    init.platformData = pd;
    bgfx::init(init);

    // 4. ImGui Initialisierung
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForOther(window);
    ImGui_Implbgfx_Init(255);

    // 5. Main Loop
    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) quit = true;
        }

        // --- Hier lagen deine Fehlerzeilen ---
        ImGui_Implbgfx_NewFrame();
        ImGui_ImplSDL3_NewFrame(); // <--- War zuvor außerhalb der Schleife
        ImGui::NewFrame();

        ImGui::Begin("Monitor");
        ImGui::Text("CPU: %.1f %%", myCpuValue);
        ImGui::End();

        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        bgfx::frame(); // <--- War zuvor außerhalb der Schleife
        // --------------------------------------
    } 

    // 6. Cleanup
    ImGui_Implbgfx_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}