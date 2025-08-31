
// System
#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <ctime>

#include "asserted.h"
#include "imgui.h"
#include "imgui_internal.h"  // For the DockBuilder API (alpha) + title bar tooltip
#include "implot.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ASSERT(x) asserted(x)
#include "stb_image.h"  // To load the icon

// Internal
#include <math.h>

#include "appCommon.h"
#include "main.h"
#include "os.h"
#include "platform.h"

#define DOCKSPACE_FLAGS ImGuiDockNodeFlags_PassthruCentralNode

// Embedded with Dear Imgui binary_to_compressed_c.cpp tool:'a.out -u32 "docs/images/icon.png" icon'
static const unsigned int icon_size           = 1492;
static const unsigned int icon_data[1492 / 4] = {
    0x474e5089, 0x0a1a0a0d, 0x0d000000, 0x52444849, 0x20000000, 0x20000000, 0x00000608, 0x7a7a7300, 0x000000f4, 0x474b6206, 0x00ae0044,
    0xada300aa, 0x006cb064, 0x70090000, 0x00735948, 0x00232e00, 0x01232e00, 0x763fa578, 0x74050000, 0x54414449, 0x96b5c358, 0x675d884f,
    0xef9f8719, 0xe6ffb9df, 0xef7b9dce, 0xa4d04da4, 0x45d268ed, 0x5621b49a, 0x0b85d224, 0xa0ad452d, 0x8828b558, 0xc5170514, 0x8a42ec6c,
    0x52aceeb4, 0x2a2d050b, 0x22845da2, 0x8b10a2a2, 0xc52c6a14, 0x3538b528, 0x2484c9b4, 0x999b5126, 0x73b9933b, 0x9ce7b9ee, 0x770bafef,
    0xbd24d4d2, 0x0befb679, 0x07efc3df, 0x0ae0af1f, 0x1bfcfa3c, 0xfd86caed, 0x23bf32d3, 0xe57a4c9f, 0x326dbf24, 0xa2abacbe, 0xf12cae10,
    0xe50a42be, 0x422b0853, 0x7b568d8a, 0x9e17d5ee, 0xd71afc38, 0xc0d2be88, 0x05dc74c5, 0x110b877b, 0xb5a53de7, 0x956049d4, 0x124519c0,
    0xa4229486, 0x510ef444, 0xcdfbdc57, 0xfe7c1d1f, 0x3afe87b1, 0x8f7f8113, 0xe6c2e646, 0x860f372a, 0x9fe186b8, 0x193a21c2, 0x8009ae62,
    0x28101124, 0xb8948d04, 0xa73bd5a9, 0x5998dbac, 0xf74efd02, 0x85b9c5c5, 0x01c307db, 0x5f4264f9, 0x10810077, 0x09409290, 0x12915ad0,
    0x9494c594, 0xbe4fcdf8, 0xd9b9cf7a, 0x9de21055, 0x69935b71, 0x89235047, 0xc1a6e864, 0x02944f7b, 0x194235a9, 0x59c21c28, 0xb8fb0da2,
    0x2028ae0e, 0xd032a610, 0x839c096c, 0x97706f38, 0x9061322b, 0xb26aaa40, 0x1a684305, 0x44290912, 0xe4047af7, 0x994a0695, 0x7c7ade33,
    0x124ca408, 0x0c0fc043, 0x76c476d3, 0x09a7a104, 0x0848620a, 0x9e52509f, 0x3340a999, 0x16907f33, 0xc2c48fa2, 0x4e911143, 0xc6c7ac10,
    0x251e7380, 0x82244904, 0xefcda300, 0xe0902a67, 0x9942ac88, 0x025305e7, 0x53196d63, 0x2ea24d62, 0x28ec622c, 0x97a60bd7, 0xf32148c6,
    0xafcfcc24, 0xddbc4047, 0xfebf2bf0, 0x3dcba3fb, 0x7cf9f8f7, 0x957d7a9d, 0x97e451b6, 0x435a6427, 0x41608884, 0xded43937, 0x7e9fedf5,
    0xcc8f03fe, 0x96800134, 0xadb9df17, 0x1ab74ec8, 0x8e7b522d, 0x8ee80849, 0xca9e4aa1, 0xec32b47b, 0x195f6d6d, 0xb9c7dd64, 0xe056de4e,
    0x5bda9f7b, 0xfee6abbe, 0x6e3ba17d, 0x29943b35, 0x9f1ed791, 0xec654a04, 0xdadb6c6f, 0x7d1fbaae, 0xc5bd81f2, 0x7800110d, 0x5f7b93e2,
    0x47c5cc5f, 0xa29d776e, 0xd281ead3, 0xd0386498, 0xad4bd618, 0xf07ec7e1, 0xde1581e0, 0x966af201, 0xbec0f87e, 0xdf9e2f3f, 0x5ad2f2f8,
    0xdef3418d, 0x9538b23c, 0x67fefa5f, 0x7791fc0e, 0xe596b888, 0xbbfd3c2f, 0x62ebaf93, 0xbf52eecf, 0x27f0e472, 0x900cc7de, 0x601cb2d7,
    0x910de834, 0x11988f4a, 0xd0f02d57, 0xa9dd8f8f, 0x77bbedbb, 0xd59b2eca, 0x3e2fdee4, 0x5981c2f3, 0x05555c08, 0xb27a3adf, 0x7fb536b7,
    0xb77b0c1b, 0x8eb4ba5c, 0xd7570e77, 0xeddae667, 0xdcfe9ebb, 0x2786fcc1, 0x8ead3ab0, 0xb7fb170c, 0xa52595d4, 0xa5129484, 0xaab4d2d4,
    0xa5e67b9f, 0x0a86e20b, 0xe58bfc3c, 0x79b76447, 0x5835eb9f, 0xbcc51593, 0x94d03260, 0xa75e198b, 0x3e9f8efb, 0x561b8dff, 0x7abfc3c1,
    0xc21f56ed, 0x1d9ec689, 0xab02976d, 0xa6741b9b, 0xcf815509, 0x186b4d20, 0xd3a17197, 0x9f53ff7e, 0xcccaf83d, 0x3d214813, 0x76edfb25,
    0x25545092, 0x346cf475, 0xf85cf91d, 0x36d00def, 0x5e652948, 0xbcce4f56, 0xf2fdaf82, 0xe10852c4, 0xe16dd17e, 0x2b6a67bc, 0x9d1852a4,
    0x8e817991, 0xa634ad09, 0x78c420d1, 0x96e7e1cf, 0x22f4cd0f, 0x2920f04a, 0x508276d1, 0x22b6167a, 0xd1224121, 0x03e90a70, 0x0a5580c6,
    0x555b1527, 0xe5e061f1, 0x76905619, 0xb951292a, 0x15161545, 0x7778ff24, 0x0cbba397, 0x41255b27, 0x94a04201, 0x528d6b46, 0x2699aeea,
    0x71366ad0, 0xb81151ce, 0xbf835378, 0xaa31cc2c, 0x29ada426, 0xa71c372f, 0xd121c152, 0xd06e8767, 0xdb14b402, 0x63b02a67, 0xca2f09d8,
    0x143699c8, 0x767492d3, 0xf1cf3f44, 0x569a4179, 0x2638e0f8, 0xb34c9847, 0x4f7974db, 0xa66a7fb3, 0x7f2b1702, 0x231aa372, 0xb2a8e7ce,
    0x5c0b4dc4, 0x05ac1159, 0x22de496b, 0xb8092252, 0x6fe93660, 0x999a7f53, 0x9e8fd7c0, 0xf149fa1a, 0xbfe6ff41, 0x65472698, 0x04b6be2d,
    0x23ac135b, 0x1de88ad9, 0x15b04b58, 0x92c28ed4, 0x62fc7b3f, 0xbe62d2d6, 0x98c449f4, 0xbb61a5aa, 0x168cb5e8, 0x88999a48, 0xa636104a,
    0x60abccb5, 0x8fda3626, 0xbe39bfef, 0xea3680a5, 0x9fb4be43, 0xf45a63de, 0x35653947, 0xa9c93265, 0x2baf855d, 0x2190ada2, 0xc5205c90,
    0x2834c340, 0x1ecfebc6, 0x99f95e5b, 0x9652766c, 0x55699899, 0x23268cac, 0x52d3b525, 0x9b494ce9, 0x84479129, 0x173591f2, 0xf1c063d3,
    0xd6b014b7, 0x5fbc11bd, 0x837dc856, 0x4da8bef9, 0xa4d66e93, 0x5d424b2a, 0xb7856de1, 0x82382135, 0xb16778f3, 0x3468c955, 0x490b7312,
    0x46474ec4, 0x6224bd1a, 0x2f4593af, 0xb884dc85, 0x33ac9311, 0xb535ce25, 0x17102138, 0xfc57effe, 0x6abedfdd, 0xbb3d9f4e, 0x79c5dd7b,
    0x935db263, 0xdafcfaf1, 0xadb525ea, 0xfa0e69d3, 0x3baf7683, 0x569420bf, 0x55a83bba, 0xae737ad6, 0x4a5758d5, 0xdea6bde6, 0x18d0b9dc,
    0x6d7437cf, 0x720cb8a4, 0xbeba64cb, 0xfa5ed6fb, 0x03fe37df, 0x98c442c0, 0xbe6fbd74, 0x00000000, 0x444e4549, 0x826042ae,
};

// ==============================================================================================
// Base application windows
// ==============================================================================================

void
appMain::drawMainMenuBar()
{
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load logs", nullptr, false, _phase == Phase::WaitForFilename)) { _fileDialogLoadLogs->open(_lastPath); }

            if (ImGui::MenuItem("Clear", nullptr, false, _phase == Phase::Active)) {
                _textViews.clear();
                _filename = "";
                _phase    = Phase::WaitForFilename;
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) { _platform->quit(); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            if (ImGui::MenuItem("New text view")) { addTextView(getId()); }

            ImGui::Separator();
            ImGui::MenuItem("Settings", nullptr, &_settingsView.isOpen);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) { _showAbout = true; }
            ImGui::EndMenu();
        }

#if 1
        // Draw the FPS at the top right
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("FPS: 9999 ").x - style.WindowPadding.x - style.ItemSpacing.x);
        ImGui::Text("FPS: %-4d", (int)(1000000 / bsMax(101LL, _platform->getLastRenderingDuration())));
#endif

        ImGui::EndMenuBar();
    }  // End of the menu bar

    // Handle the load file dialog
    if (_fileDialogLoadLogs->draw()) { dirty(); }
    if (_fileDialogLoadLogs->hasSelection()) {
        const std::vector<bsString>& result = _fileDialogLoadLogs->getSelection();
        if (!result.empty()) {
            _lastPath = result[0];
            _filename = result[0];
        }
        _fileDialogLoadLogs->clearSelection();
    }
}

void
appMain::drawAbout()
{
    static constexpr char const* textDescr = "ssview - graphical log viewer";

    if (!_showAbout) return;
    float fontSize     = ImGui::GetFontSize();
    float bigTextWidth = ImGui::CalcTextSize(textDescr).x + 4.f * fontSize;
    ImGui::SetNextWindowSize(ImVec2(bigTextWidth, fontSize * 13.f));
    if (!ImGui::Begin("ssview - About", &_showAbout,
                      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::End();
        return;
    }
    float winX     = ImGui::GetWindowPos().x;
    float winY     = ImGui::GetWindowPos().y;
    float winWidth = ImGui::GetWindowContentRegionMax().x;

    // Bold colored title
    ImU32 titleBg   = IM_COL32(255, 200, 200, 255);
    ImU32 titleFg   = IM_COL32(50, 150, 255, 255);
    float textWidth = ImGui::CalcTextSize("ssview").x;
    float x = winX + 0.5f * (winWidth - 2.f * textWidth), y = winY + 2.f * fontSize;
    DRAWLIST->AddText(ImGui::GetFont(), 2.f * fontSize, ImVec2(x - 0.1f * fontSize, y - 0.1f * fontSize), titleBg, "ssview");
    DRAWLIST->AddText(ImGui::GetFont(), 2.f * fontSize, ImVec2(x, y), titleFg, "ssview");
    y += 2.f * fontSize;

#define TEXT_POSITION(text, lineSpan, coefScreenWidth, coefTextWidth)                                                                \
    DRAWLIST->AddText(ImVec2(winX + (coefScreenWidth) * winWidth + (coefTextWidth) * ImGui::CalcTextSize(text).x, y), uWhite, text); \
    y += (lineSpan) * fontSize

    // Description
    TEXT_POSITION("", 1, 0.5f, -0.5f);
    TEXT_POSITION(textDescr, 3, 0.5f, -0.5f);
    TEXT_POSITION("(coin coin)", 2, 0.5f, -0.5f);  // @FIXME

    // Buttons
    ImGui::SetCursorPosY(fontSize * 10.5f);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(0.2f * winWidth);
    ImGui::SameLine(0.7f * winWidth);
    if (ImGui::Button("Close")) _showAbout = false;

    ImGui::End();
}

void
appMain::drawSettings()
{
    static int  draggedFontSize = -1;
    const float sliderWidth     = ImGui::CalcTextSize("UTC").x * 5.f;  // Arbitrary size
    const float titleWidth      = ImGui::CalcTextSize("UTC date (else localtime)").x + 0.25f * sliderWidth;

    SettingsView& sv = _settingsView;

    if (!sv.isOpen) { return; }
    if (sv.uniqueId == 0) {
        sv.uniqueId = getId();
        selectBestDockLocation(true, false);
    }

    char tmpStr[128];
    snprintf(tmpStr, sizeof(tmpStr), "Settings###%d", _settingsView.uniqueId);
    if (!ImGui::Begin(tmpStr, &sv.isOpen, ImGuiWindowFlags_NoCollapse)) {
        draggedFontSize = -1;
        ImGui::End();
        return;
    }

    if (ImGui::BeginTable("##tableSettings", 2)) {
        ImPlotStyle& plotStyle = ImPlot::GetStyle();
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, titleWidth);

        // UTC or local time
        ImGui::TableNextColumn();
        ImGui::Text("Local time date (else UTC)");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##Local time usage", &_settingsView.useLocalTime)) { plotStyle.UseLocalTime = _settingsView.useLocalTime; }
        ImGui::Spacing();

        // Date format
        ImGui::TableNextColumn();
        ImGui::Text("Date format");
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("The format of the date used in text views."); }
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::PushItemWidth(sliderWidth);
        int timeFormatInt = (int)sv.timeFormat;
        if (ImGui::Combo("##DateFormat", &timeFormatInt, "ss.ns\0ss.µs\0hh:mm:ss.ns\0hh:mm:ss.µs\0\0")) {
            sv.timeFormat = TimeFormat{timeFormatInt};
        }
        ImGui::PopItemWidth();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // Font size
        ImGui::TableNextColumn();
        ImGui::Text("Font size");
        ImGui::TableNextColumn();
        if (draggedFontSize < 0) draggedFontSize = _settingsView.fontSize;
        ImGui::SetNextItemWidth(sliderWidth);
        ImGui::SliderInt("##Font size", &draggedFontSize, FontSizeMin, FontSizeMax, "%d", ImGuiSliderFlags_ClampOnInput);
        if (draggedFontSize >= 0 && !ImGui::IsMouseDown(0)) {
            if (draggedFontSize != _settingsView.fontSize) { _settingsView.fontSize = draggedFontSize; }
            draggedFontSize = -1;
        }
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // color seed
        ImGui::TableNextColumn();
        ImGui::Text("Random color seed");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(sliderWidth);
        ImGui::DragInt("##Color seed", &_settingsView.colorSeed, 1.0f, 0, 1000, "%d",
                       ImGuiSliderFlags_ClampOnInput | ImGuiSliderFlags_WrapAround);

        // Line weight
        ImGui::TableNextColumn();
        ImGui::Text("Plot line width");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(sliderWidth);
        ImGui::SliderFloat("##Plot line width", &plotStyle.LineWeight, 1., 8., "%.1f", ImGuiSliderFlags_ClampOnInput);
        ImGui::Spacing();

        ImGui::EndTable();
    }

    ImGui::End();
}

// ==============================================================================================
// Main application
// ==============================================================================================

appMain::appMain(appPlatform* platform, const bsString& filename) : _platform(platform)
{
    // Create internal objects
    _fileDialogLoadLogs = new appFileDialog("Select sslog folder", appFileDialog::SELECT_DIR, {"*.*"});

    // Install the icon
    int      width = 0, height = 0;
    uint8_t* pixels = stbi_load_from_memory((const stbi_uc*)icon_data, icon_size, &width, &height, nullptr, 4);

    os::setIcon(width, height, pixels);  // The array is owned by the OS layer now
    free(pixels);

    // Palette is computed from a selection of 8 well separated (=for the eyes) hues
    // Each hue provides 4 colors: bright saturated, bright pastel, dim saturated, dim pastel
    constexpr float hues[8] = {40., 60., 96., 175., 210., 240., 280., 310.};  // In degrees
    for (int i = 0; i < 32; ++i) {
        int   i8 = i % 8, i16 = i % 16;
        float r = NAN, g = NAN, b = NAN;

        // Create the color from the hue modulo 8. Some adjustment are required due to perceptual
        float h = hues[i8] / 360.f;
        float s = ((i & 0x8) == 0) ? 1.0f : 0.5f;
        float v = ((i & 0x10) == 0) ? 1.0f : 0.55f;
        if (i < 16 && (i == 8 || i8 == 1 || i8 == 2 || i8 == 3))
            v -= 0.2f;  // Yellow, green, cyan too bright
        else if (i16 == 5 || i16 == 6 || i16 == 7)
            s -= 0.1f;  // Dark blue, violet and magenta  are too saturated

        // Build the dark and light colors from the average one
        ImGui::ColorConvertHSVtoRGB(h, s, bsMin(1.0f, 1.2f * v), r, g, b);  // Boost a bit the value for light color
        _colorPalette.push_back((ImU32)ImColor(r, g, b));
    }

    // Configure implot
    ImPlotStyle& plotStyle   = ImPlot::GetStyle();
    plotStyle.Use24HourClock = true;
    plotStyle.UseLocalTime   = _settingsView.useLocalTime;

    // Process the filename parameter
    _filename = filename;
}

appMain::~appMain() { delete _fileDialogLoadLogs; }

void
appMain::notifyStart()
{
}

int
appMain::getDisplayWidth()
{
    return _platform->getDisplayWidth();
}

int
appMain::getDisplayHeight()
{
    return _platform->getDisplayHeight();
}

void
appMain::dirty()
{
    _platform->notifyDrawDirty();
}

bool
appMain::loadSession()
{
    if (!_logSession.init(_filename.toChar(), _fileLoadErrorMsg)) { return false; }

    // Get the time zone offset compared to UTC
    time_t    now = time(nullptr);
    struct tm lcl = *localtime(&now);
    struct tm gmt = *gmtime(&now);
    _tzOffsetNs   = (int64_t)(lcl.tm_hour - gmt.tm_hour) * 3600000000000LL;

    // Get the log origin
    std::vector<sslogread::Rule> rules;

    // Collect the logs and stop after the first message
    if (!_logSession.query(
            rules,
            [this](int /*ruleIdx*/, const sslogread::LogStruct& log) {
                _originUtcNs             = log.timestampUtcNs;
                time_t     originDateSec = (time_t)((_originUtcNs + _tzOffsetNs) / 1000000000LL);
                struct tm* t             = gmtime(&originDateSec);
                asserted(t);
                _originDate = {1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec};
                _dayOriginUtcNs =
                    1000000000LL * (t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec) + ((_originUtcNs + _tzOffsetNs) % 1000000000LL);
                return false;
            },
            _fileLoadErrorMsg)) {
        return false;
    }

    return true;
}

void
appMain::draw()
{
    ImGui::PushFont(nullptr, _settingsView.fontSize);
    _lastMouseMoveDurationUs = _platform->getLastMouseMoveDurationUs();

    // Create the global window
    ImGuiIO&    io    = ImGui::GetIO();
    const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoNavFocus |
                        ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    if (!ImGui::Begin("App window", nullptr, flags | ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    // Docking
    ImGuiID mainDockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(mainDockspaceId, ImVec2(0.0f, 0.0f), DOCKSPACE_FLAGS);

    // Draw all windows
    drawMainMenuBar();
    drawAbout();
    drawSettings();

    switch (_phase) {
        case Phase::WaitForFilename:
            if (!_filename.empty()) { _phase = Phase::InitiateFileLoading; }
            break;

        case Phase::InitiateFileLoading: {
            if (!loadSession()) {
                fprintf(stderr, "Error: %s\n", _fileLoadErrorMsg.c_str());
                ImGui::OpenPopup("Unable to load logs");
                _filename = "";
                _phase    = Phase::WaitForFilename;
                break;
            }
            addTextView(getId());
            _phase = Phase::Active;
            break;
        }

        case Phase::Active:
            drawTexts();
            drawPlots();
            break;

        case Phase::InitFont:
            _phase = Phase::WaitForFilename;
            break;

        default:
            asserted(false);
    }

    // Popup for loading
    bool openPopupModal = true;
    if (ImGui::BeginPopupModal("Unable to load logs", &openPopupModal, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Text, uYellow);
        ImGui::Text("Error: %s", _fileLoadErrorMsg.c_str());
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(0.4f * ImGui::GetWindowContentRegionMax().x, ImGui::GetFontSize() * 5.f));
        if (ImGui::Button("Close")) {
            _filename = "";
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Handle the font size hotkeys globally
    if (ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_KeypadAdd) && _settingsView.fontSize < FontSizeMax) { _settingsView.fontSize += 1; }
        if (ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract) && _settingsView.fontSize > FontSizeMin) { _settingsView.fontSize -= 1; }
    }
    ImGui::End();  // End of global window

    ImGui::PopFont();
}
