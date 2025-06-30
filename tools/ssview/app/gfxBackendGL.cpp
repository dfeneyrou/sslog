
// This file implements the Open GL backend

#if defined(__linux__) || defined(_WIN32)

// External
#include "imgui.h"

// Internal
#include "bs.h"
#include "bsVec.h"
#include "gfxBackend.h"
#include "glHelper.h"

// Shaders
static const GLchar* guiVertexShaderSrc =
    "#version 300 es\n"
    "uniform mat4 ProjMtx;\n"
    "layout (location = 0) in vec2 Position;\n"
    "layout (location = 1) in vec2 UV;\n"
    "layout (location = 2) in vec4 Color;\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "    Frag_UV = UV;\n"
    "    Frag_Color = Color;\n"
    "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
    "}\n";

static const GLchar* guiFragmentShaderSrc =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D Texture;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "layout (location = 0) out vec4 Out_Color;\n"
    "void main()\n"
    "{\n"
    "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
    "}\n";

// Rendering context
static struct {
    int               frameBufferWidth  = 0;
    int               frameBufferHeight = 0;
    gfx::GlProgramVAO guiGlProgram;
    GLuint            fontTextureId             = 0;
    int               unifAttribLocationTex     = 0;
    int               unifAttribLocationProjMtx = 0;
    int               attribLocationPosition    = 0;
    int               attribLocationUV          = 0;
    int               attribLocationColor       = 0;
} vwGlCtx;

void
vwBackendInit(void)
{
    // Set maximum texture size (for ImGui dynamic fonts)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    GLint            maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    platform_io.Renderer_TextureMaxWidth = platform_io.Renderer_TextureMaxHeight = (int)maxTextureSize;

    // Allocate the font texture (fully initialized later)
    glGenTextures(1, &vwGlCtx.fontTextureId);
    glBindTexture(GL_TEXTURE_2D, vwGlCtx.fontTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    // Build and configure the OpenGL Vertex Array Object for GUI
    vwGlCtx.guiGlProgram.install(guiVertexShaderSrc, guiFragmentShaderSrc);
    vwGlCtx.unifAttribLocationTex     = glGetUniformLocation(vwGlCtx.guiGlProgram.getId(), "Texture");
    vwGlCtx.unifAttribLocationProjMtx = glGetUniformLocation(vwGlCtx.guiGlProgram.getId(), "ProjMtx");
    vwGlCtx.attribLocationPosition    = glGetAttribLocation(vwGlCtx.guiGlProgram.getId(), "Position");
    vwGlCtx.attribLocationUV          = glGetAttribLocation(vwGlCtx.guiGlProgram.getId(), "UV");
    vwGlCtx.attribLocationColor       = glGetAttribLocation(vwGlCtx.guiGlProgram.getId(), "Color");
    glEnableVertexAttribArray(vwGlCtx.attribLocationPosition);
    glEnableVertexAttribArray(vwGlCtx.attribLocationUV);
    glEnableVertexAttribArray(vwGlCtx.attribLocationColor);
    glVertexAttribPointer(vwGlCtx.attribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(vwGlCtx.attribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(vwGlCtx.attribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                          (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
    GL_CHECK();

    // Base GL setup
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
}

void
vwBackendDestroyTexture(ImTextureData* tex)
{
    GLuint gl_tex_id = (GLuint)(intptr_t)tex->TexID;
    glDeleteTextures(1, &gl_tex_id);
    tex->SetTexID(ImTextureID_Invalid);
    tex->SetStatus(ImTextureStatus_Destroyed);
    GL_CHECK();
}

void
vwBackendUpdateTexture(ImTextureData* tex)
{
    if (tex->GetStatus() == ImTextureStatus_WantCreate) {
        // Create and upload new texture to graphics system
        // IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        asserted(tex->TexID == 0 && tex->BackendUserData == nullptr);
        asserted(tex->Format == ImTextureFormat_RGBA32);
        const void* pixels        = tex->GetPixels();
        GLuint      gl_texture_id = 0;

        // Upload texture to graphics system
        // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or
        // 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGenTextures(1, &gl_texture_id);
        glBindTexture(GL_TEXTURE_2D, gl_texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->Width, tex->Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // Store identifiers
        tex->SetTexID((ImTextureID)(intptr_t)gl_texture_id);
        tex->SetStatus(ImTextureStatus_OK);

        // Restore state
        glBindTexture(GL_TEXTURE_2D, last_texture);
        GL_CHECK();
    } else if (tex->GetStatus() == ImTextureStatus_WantUpdates) {
        // Update selected blocks. We only ever write to textures regions which have never been used before!
        // This backend choose to use tex->Updates[] but you can use tex->UpdateRect to upload a single region.
        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

        GLuint gl_tex_id = (GLuint)(intptr_t)tex->TexID;
        glBindTexture(GL_TEXTURE_2D, gl_tex_id);

        bsVec<uint8_t> tempBuffer;
        for (ImTextureRect& r : tex->Updates) {
            const int src_pitch = r.w * tex->BytesPerPixel;
            tempBuffer.resize(r.h * src_pitch);
            uint8_t* out_p = tempBuffer.data();
            for (int y = 0; y < r.h; y++, out_p += src_pitch) { memcpy(out_p, tex->GetPixelsAt(r.x, r.y + y), src_pitch); }
            IM_ASSERT(out_p == tempBuffer.data() + tempBuffer.size());
            glTexSubImage2D(GL_TEXTURE_2D, 0, r.x, r.y, r.w, r.h, GL_RGBA, GL_UNSIGNED_BYTE, tempBuffer.data());
        }
        tex->SetStatus(ImTextureStatus_OK);
        glBindTexture(GL_TEXTURE_2D, last_texture);  // Restore state
        GL_CHECK();
    } else if (tex->GetStatus() == ImTextureStatus_WantDestroy && tex->UnusedFrames > 0) {
        vwBackendDestroyTexture(tex);
    }
}

bool
vwBackendDraw(void)
{
    ImDrawData* drawData = ImGui::GetDrawData();
    asserted(drawData);
    vwGlCtx.frameBufferWidth  = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    vwGlCtx.frameBufferHeight = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (vwGlCtx.frameBufferWidth == 0 || vwGlCtx.frameBufferHeight == 0) return false;

    // Catch up with texture updates. Most of the times, the list will have 1 element with an OK status, aka nothing to do.
    // (This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture
    // updates).
    if (drawData->Textures != nullptr) {
        for (ImTextureData* tex : *drawData->Textures) {
            if (tex->GetStatus() != ImTextureStatus_OK) { vwBackendUpdateTexture(tex); }
        }
    }

    // Backup GL state
    // vwGlBackupState backup;
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    GL_CHECK();

    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    GL_CHECK();

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)vwGlCtx.frameBufferWidth, (GLsizei)vwGlCtx.frameBufferHeight);
    ImGuiIO&    io                     = ImGui::GetIO();
    const float ortho_projection[4][4] = {
        {2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
    };
    glUseProgram(vwGlCtx.guiGlProgram.getId());
    glUniform1i(vwGlCtx.unifAttribLocationTex, 0);
    glUniformMatrix4fv(vwGlCtx.unifAttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(vwGlCtx.guiGlProgram.getVaoId());
    glBindSampler(0, 0);  // Rely on combined texture/sampler state.
    GL_CHECK();

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clipOff   = drawData->DisplayPos;        // (0,0) unless using multi-viewports
    ImVec2 clipScale = drawData->FramebufferScale;  // (1,1) unless using retina display which are often (2,2)

    glBindBuffer(GL_ARRAY_BUFFER, vwGlCtx.guiGlProgram.getVboId());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vwGlCtx.guiGlProgram.getIboId());

    for (int n = 0; n < drawData->CmdListsCount; ++n) {
        const ImDrawList* cmdList = drawData->CmdLists[n];

        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmdList->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmdList->VtxBuffer.Data,
                     GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx),
                     (const GLvoid*)cmdList->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; ++cmdIdx) {
            const ImDrawCmd* cmd = &cmdList->CmdBuffer[cmdIdx];
            if (cmd->UserCallback) {
                cmd->UserCallback(cmdList, cmd);
            } else if (cmd->ElemCount) {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clipMin((cmd->ClipRect.x - clipOff.x) * clipScale.x, (cmd->ClipRect.y - clipOff.y) * clipScale.y);
                ImVec2 clipMax((cmd->ClipRect.z - clipOff.x) * clipScale.x, (cmd->ClipRect.w - clipOff.y) * clipScale.y);
                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
                glScissor((int)clipMin.x, (int)((float)vwGlCtx.frameBufferHeight - clipMax.y), (int)(clipMax.x - clipMin.x),
                          (int)(clipMax.y - clipMin.y));

                // Bind texture, Draw
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)cmd->GetTexID());
                glDrawElements(GL_TRIANGLES, (GLsizei)cmd->ElemCount, GL_UNSIGNED_INT,
                               (void*)(intptr_t)(cmd->IdxOffset * sizeof(ImDrawIdx)));
            }
        }
    }

    // Restore
    glDisable(GL_SCISSOR_TEST);
    GL_CHECK();
    // backup.restore();

    return true;  // We drew something
}

bool
vwCaptureScreen(int* width, int* height, uint8_t** buffer)
{
    if (vwGlCtx.frameBufferWidth == 0 || vwGlCtx.frameBufferHeight == 0) return false;
    asserted(width && height && buffer);
    *width  = vwGlCtx.frameBufferWidth & ~0x3;  // Ensure multiple of 4 for better compatibility
    *height = vwGlCtx.frameBufferHeight;
    *buffer = new uint8_t[3 * (*width) * (*height)];  // RGB = 3 components
    glReadPixels(0, 0, *width, *height, GL_RGB, GL_UNSIGNED_BYTE, *buffer);
    return true;
}

void
vwBackendInstallFont(const void* fontData, int fontDataSize, int fontSize)
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.FontDefault = io.Fonts->AddFontFromMemoryCompressedTTF(fontData, fontDataSize, (float)fontSize);
}

void
vwBackendUninit(void)
{
    vwGlCtx.guiGlProgram.deinstall();

    // Destroy all textures
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures) {
        if (tex->RefCount == 1) { vwBackendDestroyTexture(tex); }
    }
}

#endif  // if defined(__linux__) || defined(_WIN32)
