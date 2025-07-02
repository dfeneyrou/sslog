// This file implements the base application glue directly on top of the OS, a bit like a "main".
// Drawing is subcontracted to the graphic backend

// System
#include <cstdio>
#include <cstdlib>
#include <cstring>

// External
#include "imgui.h"

// Internal
#define PL_IMPLEMENTATION 1
#include "bsTime.h"
#include "fontData.h"
#include "gfxBackend.h"
#include "main.h"
#include "os.h"
#include "platform.h"

// Parameters
const int RENDER_FRAME_US      = 16000;
const int BOUNCE_RENDER_GAP_US = 500000;  // 0.5 second bounce

// Clipboard wrappers for ImGui
static const char*
vwGetClipboardText(void* user_data)
{
    (void)user_data;
    static bsString lastString;
    lastString = os::reqFromClipboard(os::ClipboardType::UTF8).toUtf8();
    return lastString.toChar();
}

static void
vwSetClipboardText(void* user_data, const char* text)
{
    (void)user_data;
    os::pushToClipboard(os::ClipboardType::UTF8, bsString(text).toUtf16());
}

// ==============================================================================================
// Entry point
// ==============================================================================================

int
osBootstrapImpl(int argc, char* argv[])
{
    // Parse arguments
    bool     doDisplayHelp = false;
    bsString filename;
    int      i = 1;
    while (i < argc) {
        if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "/help") ||
            !strcmp(argv[i], "/?")) {
            doDisplayHelp = true;
        } else if (argv[i][0] == '-') {
            printf("ERROR: Unknown option '%s'\n", argv[i]);
            doDisplayHelp = true;
        } else if (!filename.empty()) {
            printf("ERROR: Only one filename shall be provided, not '%s' and '%s'\n", filename.toChar(), argv[i]);
            doDisplayHelp = true;
        } else {
            filename = argv[i];
        }

        // Next argument
        ++i;
    }

    if (doDisplayHelp) {
        printf("ssview: a sslog GUI to easily display filtered logs and graph values\n");
        printf(" Syntax :  ssview [options] <log folder>\n");
        printf(" Options:\n");
        printf("  -h or --help      dumps this help\n");
        return 1;
    }

    // Init
    os::createWindow("ssview", "ssview", 0.3f, 0.2f, 0.8f, 0.8f);
    vwPlatform* platform = new vwPlatform(filename);

    // Run application
    platform->run();

    // Clean
    delete platform;
    os::destroyWindow();
    return 0;
}

// ==============================================================================================
// Wrapper on the OS and ImGUI
// ==============================================================================================

vwPlatform::vwPlatform(const bsString& filename) : _doExit(0), _isVisible(0), _dirtyRedrawCount(VW_REDRAW_PER_NTF)
{
    // Update ImGui
    int dpiWidth, dpiHeight;
    os::getWindowSize(_displayWidth, _displayHeight, dpiWidth, dpiHeight);
    _dpiScale = (float)dpiWidth / 96.f;  // No support of dynamic DPI change

    // Setup ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.DisplaySize                = ImVec2((float)_displayWidth, (float)_displayHeight);
    io.DisplayFramebufferScale    = ImVec2(1.f, 1.f);  // High DPI is handled with increased font size and Imgui spatial constants
    io.IniFilename                = 0;                 // Disable config file save
    io.MouseDragThreshold         = 1.;                // 1 pixel threshold to detect that we are dragging
    io.ConfigInputTextCursorBlink = false;
    configureStyle();
    ImGui::GetStyle().ScaleAllSizes(_dpiScale);
    io.FontDefault = io.Fonts->AddFontFromMemoryCompressedTTF(vwGetFontDataRobotoMedium(), vwGetFontDataSizeRobotoMedium(), 1.);

    // Install callbacks
    io.SetClipboardTextFn = vwSetClipboardText;
    io.GetClipboardTextFn = vwGetClipboardText;
    io.ClipboardUserData  = 0;

    // Initialize the graphical backend
    vwBackendInit();

    // Creation of the main application
    _main = new vwMain(this, filename);

    // Notify the start of the main application
    _main->notifyStart();
}

vwPlatform::~vwPlatform(void)
{
    delete _main;
    vwBackendUninit();
    ImGui::DestroyContext();
}

void
vwPlatform::run(void)
{
    enum { NO_EXIT, EXIT_REQUESTED, EXIT_NOW } exitState = NO_EXIT;

    while (exitState != EXIT_NOW) {
        // Inputs
        bsUs_t frameStartUs = bsGetClockUs();
        os::processInputs(this);

        // Render
        if (redraw()) { os::swapBuffer(); }

        if (exitState == NO_EXIT && _doExit.load()) {
            exitState = EXIT_REQUESTED;  // Required to have 1 frame drawn with the exit flag set
            notifyDrawDirty();
        } else if (exitState == EXIT_REQUESTED) {
            exitState = EXIT_NOW;
        }

        // Power management (frame rate limit)
        _lastRenderingDurationUs = bsGetClockUs() - frameStartUs;
        bsUs_t sleepDurationUs   = RENDER_FRAME_US - _lastRenderingDurationUs;
        if (sleepDurationUs > 0) { bsSleep(sleepDurationUs); }
    }
}

bool
vwPlatform::redraw()
{
    // Filter out some redraw based on the dirtiness of the display state.
    // Dear Imgui requires several frames to handle user events properly, so we display per batch.
    // Also a "bounce" is required after a delay for some tooltip to appear, even if no user event occurs.
    bsUs_t   currentTimeUs    = bsGetClockUs();
    uint64_t tmp              = _dirtyRedrawCount.load();
    int      dirtyRedrawCount = (int)(tmp & 0xFFFFFFFF);
    int      bounceCount      = (int)((tmp >> 32) & 0xFFFFFFFF);
    if (dirtyRedrawCount <= 0 && !(bounceCount == 1 && currentTimeUs - _lastRenderingTimeUs >= BOUNCE_RENDER_GAP_US)) {
        return false;  // Display is not dirty and it is not a bounce time: nothing to display
    }
#define WRITE_DIRTY_COUNT(drc, bc) _dirtyRedrawCount.store((((uint64_t)(bc)) << 32) | ((uint32_t)((drc) & 0xFFFFFFFF)))
    if (dirtyRedrawCount >= 0) {
        if (dirtyRedrawCount > 0) --dirtyRedrawCount;
        if (dirtyRedrawCount == 0) {
            bounceCount++;
            if (bounceCount == 2) {
                WRITE_DIRTY_COUNT(VW_REDRAW_PER_BOUNCE, bounceCount);
            } else {
                WRITE_DIRTY_COUNT(dirtyRedrawCount, bounceCount);
                return false;
            }
        } else
            WRITE_DIRTY_COUNT(dirtyRedrawCount, bounceCount);
    }

    // Update inputs for ImGui
    ImGuiIO& io          = ImGui::GetIO();
    io.DisplaySize       = ImVec2((float)_displayWidth, (float)_displayHeight);
    io.DeltaTime         = (_lastRenderingTimeUs == 0) ? 0.001f : 0.000001f * (float)(currentTimeUs - _lastRenderingTimeUs);
    _lastRenderingTimeUs = currentTimeUs;

    // Compute the vertices
    ImGui::NewFrame();

    _main->draw();
    ImGui::Render();
    _lastUpdateDurationUs = bsGetClockUs() - currentTimeUs;

    // Draw
    return vwBackendDraw();
}

bool
vwPlatform::captureScreen(int* width, int* height, uint8_t** buffer)
{
    return vwCaptureScreen(width, height, buffer);
}

void
vwPlatform::configureStyle(void)
{
    // Dark side of the style, as a base
    ImGui::StyleColorsDark();
    // Customization
    ImVec4* colors                        = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                 = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]             = ImVec4(0.113f, 0.117f, 0.10f, 1.00f);  // Less blue = "warmer" dark
    colors[ImGuiCol_ChildBg]              = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.15f, 0.15f, 0.15f, 0.90f);
    colors[ImGuiCol_Border]               = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.30f, 0.31f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.20f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TitleBg]              = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]            = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]            = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_SliderGrab]           = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_Button]               = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Header]               = ImVec4(1.00f, 0.70f, 0.70f, 0.31f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.75f, 0.70f, 0.70f, 0.80f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.58f, 0.50f, 0.52f, 1.00f);

    colors[ImGuiCol_Tab]                = ImVec4(0.13f, 0.24f, 0.41f, 1.f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.20f, 0.41f, 0.68f, 1.f);
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.07f, 0.10f, 0.15f, 1.f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.f);

    colors[ImGuiCol_Separator]             = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(1.00f, 0.70f, 0.70f, 0.31f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.41f, 0.41f, 0.45f, 1.00f);  // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.33f, 0.33f, 0.35f, 1.00f);  // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.30f, 0.30f, 0.30f, 0.30f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);

    ImGui::GetStyle().WindowRounding    = 2.0f;
    ImGui::GetStyle().TabRounding       = 2.0f;
    ImGui::GetStyle().ScrollbarRounding = 2.0f;
}

// ==============================================================================================
// OS event handlers
// ==============================================================================================

void
vwPlatform::notifyEnter(os::KeyModState kms)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiKey_ModCtrl, kms.ctrl);
    io.AddKeyEvent(ImGuiKey_ModShift, kms.shift);
    io.AddKeyEvent(ImGuiKey_ModAlt, kms.alt);
    io.AddKeyEvent(ImGuiKey_ModSuper, kms.sys);

    // The modifier keys are now up to date, whatver happens outside the window
    notifyDrawDirty();
}

void
vwPlatform::notifyLeave(os::KeyModState kms)
{
    // Nothing special to do
    (void)kms;
}

void
vwPlatform::eventKeyPressedOrReleased(os::Keycode keycode, os::KeyModState kms, bool pressState)
{
    asserted(keycode >= os::KC_A && keycode < os::KC_KeyCount, (int)keycode);

    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiKey_ModCtrl, kms.ctrl);
    io.AddKeyEvent(ImGuiKey_ModShift, kms.shift);
    io.AddKeyEvent(ImGuiKey_ModAlt, kms.alt);
    io.AddKeyEvent(ImGuiKey_ModSuper, kms.sys);

    switch (keycode) {
        case os::KC_Down:
            io.AddKeyEvent(ImGuiKey_DownArrow, pressState);
            break;
        case os::KC_Up:
            io.AddKeyEvent(ImGuiKey_UpArrow, pressState);
            break;
        case os::KC_Left:
            io.AddKeyEvent(ImGuiKey_LeftArrow, pressState);
            break;
        case os::KC_Right:
            io.AddKeyEvent(ImGuiKey_RightArrow, pressState);
            break;
        case os::KC_PageDown:
            io.AddKeyEvent(ImGuiKey_PageDown, pressState);
            break;
        case os::KC_PageUp:
            io.AddKeyEvent(ImGuiKey_PageUp, pressState);
            break;
        case os::KC_Add:
            io.AddKeyEvent(ImGuiKey_KeypadAdd, pressState);
            break;
        case os::KC_Subtract:
            io.AddKeyEvent(ImGuiKey_KeypadSubtract, pressState);
            break;
        case os::KC_Escape:
            io.AddKeyEvent(ImGuiKey_Escape, pressState);
            break;
        case os::KC_Backspace:
            io.AddKeyEvent(ImGuiKey_Backspace, pressState);
            break;
        case os::KC_Delete:
            io.AddKeyEvent(ImGuiKey_Delete, pressState);
            break;
        default:
            break;
    }

#if 1
    // Escape: quit application. Enabled only for easier iterations when developing
    if (keycode == os::KC_Escape) {
        quit();  // Position the flag to exit the application
    }
#endif

    notifyDrawDirty();
}

void
vwPlatform::eventKeyPressed(os::Keycode keycode, os::KeyModState kms)
{
    eventKeyPressedOrReleased(keycode, kms, true);
}

void
vwPlatform::eventKeyReleased(os::Keycode keycode, os::KeyModState kms)
{
    eventKeyPressedOrReleased(keycode, kms, false);
}

void
vwPlatform::eventWheelScrolled(int x, int y, int steps, os::KeyModState kms)
{
    (void)x;
    (void)y;
    (void)kms;
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel -= (float)steps;
    _lastMouseMoveTimeUs = bsGetClockUs();
    notifyDrawDirty();
}

void
vwPlatform::eventChar(uint16_t codepoint)
{
    ImGuiIO& io = ImGui::GetIO();
    if (codepoint != 0) { io.AddInputCharacter((uint16_t)codepoint); }
    notifyDrawDirty();
}

void
vwPlatform::eventButtonPressed(int buttonId, int x, int y, os::KeyModState kms)
{
    (void)kms;
    ImGuiIO& io                = ImGui::GetIO();
    io.MouseDown[buttonId - 1] = true;
    io.MousePos                = ImVec2((float)x, (float)y);
    _lastMouseMoveTimeUs       = bsGetClockUs();
    notifyDrawDirty();
}

void
vwPlatform::eventButtonReleased(int buttonId, int x, int y, os::KeyModState kms)
{
    (void)kms;
    ImGuiIO& io                = ImGui::GetIO();
    io.MouseDown[buttonId - 1] = false;
    io.MousePos                = ImVec2((float)x, (float)y);
    _lastMouseMoveTimeUs       = bsGetClockUs();
    notifyDrawDirty();
}

void
vwPlatform::eventMouseMotion(int x, int y)
{
    ImGuiIO& io          = ImGui::GetIO();
    io.MousePos          = ImVec2((float)x, (float)y);
    _lastMouseMoveTimeUs = bsGetClockUs();
    notifyDrawDirty();
}
