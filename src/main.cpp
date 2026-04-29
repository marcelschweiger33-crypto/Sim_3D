#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

// 1. ZUERST das offizielle ImGui (Deine Version 1.92.7)
#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <iostream>

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return -1;

    SDL_Window* window = SDL_CreateWindow("Sim_3D | Fix", 1280, 720, SDL_WINDOW_RESIZABLE);

    // bgfx Initialisierung
    bgfx::Init init;
    init.type = bgfx::RendererType::Vulkan; 
    init.resolution.width = 1280;
    init.resolution.height = 720;
    init.resolution.reset = BGFX_RESET_VSYNC;

    bgfx::PlatformData pd;
#if defined(SDL_PLATFORM_WIN32)
    pd.nwh = (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif
    init.platformData = pd;
    bgfx::init(init);

    // ImGui Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL3_InitForOther(window);

    // WICHTIG: Den Font Atlas bauen, damit 'TexIsBuilt' true wird
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    
    // Wir vergeben eine temporäre ID, damit ImGui nicht abstürzt
    io.Fonts->SetTexID((ImTextureID)(intptr_t)1);

    // Vertex Layout für das spätere Rendering definieren
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
        .end();

    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) quit = true;
        }

        // View 0 vorbereiten
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, 1280, 720);
        bgfx::touch(0);

        // ImGui Frame starten
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        
        ImGui::Begin("Vulkan Simulator");
        ImGui::Text("Fehler 'bool type' behoben!");
        if (ImGui::Button("Exit")) quit = true;
        ImGui::End();
        
        ImGui::Render();

        // FIX: Hier lag der Fehler (Die Funktionen geben void zurück!)
        ImDrawData* drawData = ImGui::GetDrawData();
        for (int i = 0; i < drawData->CmdListsCount; ++i) {
            const ImDrawList* cmdList = drawData->CmdLists[i];

            bgfx::TransientVertexBuffer tvb;
            bgfx::TransientIndexBuffer tib;

            // Wir rufen sie einzeln auf, da sie kein bool zurückgeben
            bgfx::allocTransientVertexBuffer(&tvb, cmdList->VtxBuffer.Size, layout);
            bgfx::allocTransientIndexBuffer(&tib, cmdList->IdxBuffer.Size);

            memcpy(tvb.data, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(tib.data, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setIndexBuffer(&tib);
            bgfx::submit(0, BGFX_INVALID_HANDLE); // Zeichnen (erfordert eigentlich Shader)
        }

        bgfx::frame();
    }

    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    bgfx::shutdown();
    SDL_Quit();

    return 0;
}