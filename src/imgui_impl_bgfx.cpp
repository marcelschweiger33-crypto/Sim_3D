#include "imgui_impl_bgfx.h"
#include "imgui.h"
#include "bgfx/bgfx.h"
#include "bgfx/embedded_shader.h"
#include "bx/math.h"
#include "bx/timer.h"

// Data
static uint8_t g_View = 255;
static bgfx::TextureHandle g_FontTexture = BGFX_INVALID_HANDLE;
static bgfx::ProgramHandle g_ShaderHandle = BGFX_INVALID_HANDLE;
static bgfx::UniformHandle g_AttribLocationTex = BGFX_INVALID_HANDLE;
static bgfx::VertexLayout g_VertexLayout;

void ImGui_Implbgfx_RenderDrawLists(ImDrawData* draw_data)
{
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) return;

    uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA |
                     BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

    const bgfx::Caps* caps = bgfx::getCaps();
    float ortho[16];
    bx::mtxOrtho(ortho, 0.0f, draw_data->DisplaySize.x, draw_data->DisplaySize.y, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
    bgfx::setViewTransform(g_View, NULL, ortho);
    bgfx::setViewRect(g_View, 0, 0, (uint16_t)fb_width, (uint16_t)fb_height);

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;

        uint32_t numVertices = (uint32_t)cmd_list->VtxBuffer.size();
        uint32_t numIndices = (uint32_t)cmd_list->IdxBuffer.size();

        if ((numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, g_VertexLayout)) ||
            (numIndices != bgfx::getAvailTransientIndexBuffer(numIndices))) break;

        bgfx::allocTransientVertexBuffer(&tvb, numVertices, g_VertexLayout);
        bgfx::allocTransientIndexBuffer(&tib, numIndices);

        memcpy(tvb.data, cmd_list->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));
        memcpy(tib.data, cmd_list->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                const uint16_t xx = (uint16_t)bx::max(pcmd->ClipRect.x, 0.0f);
                const uint16_t yy = (uint16_t)bx::max(pcmd->ClipRect.y, 0.0f);
                bgfx::setScissor(xx, yy, (uint16_t)bx::min(pcmd->ClipRect.z, 65535.0f) - xx, (uint16_t)bx::min(pcmd->ClipRect.w, 65535.0f) - yy);

                bgfx::setState(state);
                // FIX: pcmd->TexID verwenden
                bgfx::TextureHandle texture = {(uint16_t)((intptr_t)pcmd->GetTexID() & 0xffff)};
                bgfx::setTexture(0, g_AttribLocationTex, texture);
                bgfx::setVertexBuffer(0, &tvb, 0, numVertices);
                bgfx::setIndexBuffer(&tib, pcmd->IdxOffset, pcmd->ElemCount);
                bgfx::submit(g_View, g_ShaderHandle);
            }
        }
    }
}

bool ImGui_Implbgfx_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    g_FontTexture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::BGRA8, 0, bgfx::copy(pixels, width * height * 4));
    
    // FIX: Expliziter Cast für ImTextureID
    io.Fonts->SetTexID((ImTextureID)(intptr_t)g_FontTexture.idx);

    return true;
}

#include "fs_ocornut_imgui.bin.h"
#include "vs_ocornut_imgui.bin.h"

static const bgfx::EmbeddedShader s_embeddedShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui), 
    BGFX_EMBEDDED_SHADER_END()
};

bool ImGui_Implbgfx_CreateDeviceObjects()
{
    bgfx::RendererType::Enum type = bgfx::getRendererType();
    g_ShaderHandle = bgfx::createProgram(
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui"),
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui"),
        true);

    g_VertexLayout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    g_AttribLocationTex = bgfx::createUniform("g_AttribLocationTex", bgfx::UniformType::Sampler);
    ImGui_Implbgfx_CreateFontsTexture();
    return true;
}

void ImGui_Implbgfx_InvalidateDeviceObjects()
{
    bgfx::destroy(g_AttribLocationTex);
    bgfx::destroy(g_ShaderHandle);

    if (bgfx::isValid(g_FontTexture)) {
        bgfx::destroy(g_FontTexture);
        ImGui::GetIO().Fonts->SetTexID(0);
    }
}

void ImGui_Implbgfx_Init(int view) { g_View = (uint8_t)(view & 0xff); }
void ImGui_Implbgfx_Shutdown() { ImGui_Implbgfx_InvalidateDeviceObjects(); }
void ImGui_Implbgfx_NewFrame() { if (!bgfx::isValid(g_FontTexture)) ImGui_Implbgfx_CreateDeviceObjects(); }