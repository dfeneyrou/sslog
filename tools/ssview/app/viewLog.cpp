
#include "imgui.h"

// Internal
#include "appCommon.h"
#include "main.h"

static constexpr int TIME_FORMAT_SECOND = 0;
static constexpr int TIME_FORMAT_HHMMSS = 1;
static constexpr int TIME_FORMAT_QTY    = 2;

int
getFormattedTimeStringCharQty(int timeFormat)
{
    constexpr int timeStrCharQtyArray[TIME_FORMAT_QTY] = {17 + 2, 21 + 2};  // 2 char of margin
    asserted(timeFormat < TIME_FORMAT_QTY, timeFormat);
    return timeStrCharQtyArray[timeFormat];
}

const char*
getFormattedTimeString(int64_t ns, int timeFormat)
{
    static char outBuf[64];
    if (timeFormat == TIME_FORMAT_HHMMSS) {
        snprintf(outBuf, sizeof(outBuf), "%02d:%02d:%02d.%03d_%03d_%03d", (int)(ns / 3600000000000LL), (int)((ns / 60000000000LL) % 60),
                 (int)((ns / 1000000000LL) % 60), (int)((ns / 1000000LL) % 1000), (int)((ns / 1000LL) % 1000), (int)(ns % 1000));
    } else {
        snprintf(outBuf, sizeof(outBuf), "%d.%03d_%03d_%03ds", (int)(ns / 1000000000LL), (int)((ns / 1000000LL) % 1000),
                 (int)((ns / 1000LL) % 1000), (int)(ns % 1000));
    }
    return outBuf;
}

// ==============================================================================================
// Log view
// ==============================================================================================

void
vwMain::addLogView(uint32_t id)
{
    _logViews.push_back({});
    _logViews.back().uniqueId = id;
}

void
vwMain::drawLogs()
{
    if (_phase != Phase::Active) { return; }
    int itemToRemoveIdx = -1;

    for (int logIdx = 0; logIdx < (int)_logViews.size(); ++logIdx) {
        auto& logView = _logViews[logIdx];
        // if(_uniqueIdFullScreen>=0 && logView.uniqueId!=_uniqueIdFullScreen) continue;

        char tmpStr[256];
        snprintf(tmpStr, sizeof(tmpStr), "Log###%d", logView.uniqueId);
        bool isOpen = true;
        if (ImGui::Begin(tmpStr, &isOpen,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs)) {
            drawLog(logView);
        }
        if (!isOpen) itemToRemoveIdx = logIdx;
        ImGui::End();
    }

    // Remove profile if needed
    if (itemToRemoveIdx >= 0) {
        _logViews.erase(_logViews.begin() + itemToRemoveIdx);
        dirty();
        // setFullScreenView(-1);
    }
}

void
vwMain::drawLog(LogView& lv)
{
    char lastDateStr[256];
    lastDateStr[0] = 0;  // No previous displayed date

    ImGui::BeginChild("log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    prepareLogData(lv);

    const float fontHeight    = ImGui::GetTextLineHeightWithSpacing();
    const float textPixMargin = ImGui::GetStyle().ItemSpacing.x;
    const float charWidth     = ImGui::CalcTextSize("0").x;
    const float winX          = ImGui::GetWindowPos().x;
    const float winY          = ImGui::GetWindowPos().y;
    const float curScrollPosX = ImGui::GetScrollX();
    const float curScrollPosY = ImGui::GetScrollY();
    // const bool isWindowHovered = ImGui::IsWindowHovered();

    float maxOffsetX  = winX;
    float y           = winY;
    float yEnd        = winY + ImGui::GetWindowSize().y + fontHeight;  // Skip the invisible log part after the window end
    int   logStartIdx = curScrollPosY / fontHeight;                    // Skip the invisible log part before the window start

    // Loop on logs
    for (int logIdx = logStartIdx; logIdx < (int)lv.cachedLogs.size() && y < yEnd; ++logIdx) {
        const LogElem& ci = lv.cachedLogs[logIdx];

        // Display the date
        float       offsetX = winX + textPixMargin - curScrollPosX;
        const char* timeStr = getFormattedTimeString(ci.timestampUtcNs, _timeFormat);
        DRAWLIST->AddText(ImVec2(offsetX, y), uWhite, timeStr);
        int changedOffset = 0;
        while (timeStr[changedOffset] && timeStr[changedOffset] == lastDateStr[changedOffset]) ++changedOffset;
        DRAWLIST->AddText(ImVec2(offsetX, y), uGrey128, timeStr, timeStr + changedOffset);
        snprintf(lastDateStr, sizeof(lastDateStr), "%s", timeStr);
        offsetX += charWidth * (float)getFormattedTimeStringCharQty(_timeFormat);

        // Display the category
        DRAWLIST->AddText(ImVec2(offsetX, y), uWhite, ci.category.c_str());
        offsetX += charWidth * 4 + lv.maxCategoryLength;

        // Display the message
        DRAWLIST->AddText(ImVec2(offsetX, y), uWhite, ci.message.c_str());
        offsetX += ImGui::CalcTextSize(ci.message.c_str()).x;

        if (offsetX > maxOffsetX) maxOffsetX = offsetX;
        y += fontHeight;
    }

    ImGui::SetCursorPos(ImVec2(maxOffsetX - winX + curScrollPosX, lv.cachedLogs.size() * fontHeight));
    ImGui::Dummy(ImVec2(0, 0));  // Need this dummy item after setting the cursor position and extending the window
    ImGui::EndChild();
}

void
vwMain::prepareLogData(LogView& lv)
{
    if (!lv.isDataDirty) { return; }
    lv.isDataDirty = false;
    lv.cachedLogs.clear();
    lv.maxCategoryLength = 1;

    // Build the filtering rules
    bool                         isFirst = true;
    std::string                  errorMessage;
    std::vector<sslogread::Rule> rules;

    // Collect the logs
    if (!_logSession.query(
            rules,
            [this, &lv, &isFirst](int /*ruleIdx*/, const sslogread::LogStruct& log) {
                // Format the string with arguments (=custom vsnprintf with our argument list, see below)
                char filledFormat[1024];
                sslogread::vsnprintfLog(filledFormat, sizeof(filledFormat), _logSession.getIndexedString(log.formatIdx), log.args,
                                        &_logSession);

                // Update statistics
                if (isFirst) {
                    lv.originUtcNs = log.timestampUtcNs;
                    isFirst        = false;
                }
                int categoryWidth = ImGui::CalcTextSize(_logSession.getIndexedString(log.categoryIdx)).x;
                if (categoryWidth > lv.maxCategoryLength) lv.maxCategoryLength = categoryWidth;

                // Store
                lv.cachedLogs.push_back({log.timestampUtcNs - lv.originUtcNs, _logSession.getIndexedString(log.categoryIdx), filledFormat});
            },
            errorMessage)) {
        fprintf(stderr, "Error: %s\n", errorMessage.c_str());
    }
}
