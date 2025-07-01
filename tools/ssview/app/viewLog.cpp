
#include "imgui.h"

// Internal
#include "appCommon.h"
#include "main.h"

void
vwMain::addLogView(uint32_t id)
{
    _logViews.push_back({});
    _logViews.back().uniqueId = id;
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

void
vwMain::drawLogs()
{
    if (_phase != Phase::Active) { return; }
    int itemToRemoveIdx = -1;

    for (int logIdx = 0; logIdx < (int)_logViews.size(); ++logIdx) {
        auto& logView = _logViews[logIdx];

        char tmpStr[256];
        snprintf(tmpStr, sizeof(tmpStr), "Log###%d", logView.uniqueId);
        bool isOpen = true;

        if (logView.isNew) {
            logView.isNew = false;
            selectBestDockLocation(false, true);
        }
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
    char tmpStr[256];
    char lastDateStr[256];
    lastDateStr[0] = 0;  // No previous displayed date

    ImGui::BeginChild("log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    prepareLogData(lv);

    const float fontHeight      = ImGui::GetTextLineHeightWithSpacing();
    const float textPixMargin   = ImGui::GetStyle().ItemSpacing.x;
    const float charWidth       = ImGui::CalcTextSize("0").x;
    const float winX            = ImGui::GetWindowPos().x;
    const float winY            = ImGui::GetWindowPos().y;
    const float curScrollPosX   = ImGui::GetScrollX();
    const float curScrollPosY   = ImGui::GetScrollY();
    const float winWidth        = ImGui::GetWindowContentRegionMax().x;
    const bool  isWindowHovered = ImGui::IsWindowHovered();
    const float mouseX          = ImGui::GetMousePos().x;
    const float mouseY          = ImGui::GetMousePos().y;

    float   maxOffsetX          = winX;
    float   y                   = winY;
    float   yEnd                = winY + ImGui::GetWindowSize().y + fontHeight;  // Skip the invisible log part after the window end
    int     logStartIdx         = curScrollPosY / fontHeight;                    // Skip the invisible log part before the window start
    int64_t newMouseTimeNs      = -1;
    int64_t mouseTimeBestTimeNs = -1;
    float   mouseTimeBestY      = -1.;

    // Loop on logs
    for (int logIdx = logStartIdx; logIdx < (int)lv.cachedLogs.size() && y < yEnd; ++logIdx) {
        const LogElem& ci = lv.cachedLogs[logIdx];

        // Display the date
        float       offsetX = winX + textPixMargin - curScrollPosX;
        const char* timeStr = getFormattedTimeString(ci.timestampUtcNs, _settingsView.timeFormat);
        DRAWLIST->AddText(ImVec2(offsetX, y), uWhite, timeStr);
        int changedOffset = 0;
        while (timeStr[changedOffset] && timeStr[changedOffset] == lastDateStr[changedOffset]) ++changedOffset;
        DRAWLIST->AddText(ImVec2(offsetX, y), uGrey128, timeStr, timeStr + changedOffset);
        snprintf(lastDateStr, sizeof(lastDateStr), "%s", timeStr);
        offsetX += charWidth * (float)getFormattedTimeStringCharQty(_settingsView.timeFormat);

        // Display the category
        DRAWLIST->AddText(ImVec2(offsetX, y), uWhite, ci.category.c_str());
        offsetX += charWidth * 4 + lv.maxCategoryLength;

        // Display the message
        DRAWLIST->AddText(ImVec2(offsetX, y), uWhite, ci.message.c_str());
        offsetX += ImGui::CalcTextSize(ci.message.c_str()).x;

        if (isWindowHovered && mouseY > y) newMouseTimeNs = ci.timestampUtcNs;
        if (_mouseTimeNs >= (int64_t)ci.timestampUtcNs && (int64_t)ci.timestampUtcNs > mouseTimeBestTimeNs) {
            mouseTimeBestTimeNs = ci.timestampUtcNs;
            mouseTimeBestY      = y + fontHeight;
        }

        if (offsetX > maxOffsetX) maxOffsetX = offsetX;
        y += fontHeight;
    }

    // Drag with middle button
    if (isWindowHovered && ImGui::IsMouseDragging(1)) {
        // Start a range selection
        if (lv.rangeSelStartNs < 0 && mouseTimeBestTimeNs >= 0) {
            lv.rangeSelStartNs = mouseTimeBestTimeNs;
            lv.rangeSelStartY  = mouseTimeBestY;
        }

        // Drag on-going: display the selection box with transparency and range
        if (lv.rangeSelStartNs >= 0 && lv.rangeSelStartNs < mouseTimeBestTimeNs) {
            float           x         = winX + textPixMargin - curScrollPosX;
            float           y1        = lv.rangeSelStartY - fontHeight;
            float           y2        = mouseTimeBestY;
            constexpr float arrowSize = 4.f;
            // White background
            DRAWLIST->AddRectFilled(ImVec2(x, y1), ImVec2(x + winWidth, y2), IM_COL32(255, 255, 255, 128));
            // Range line
            DRAWLIST->AddLine(ImVec2(mouseX, y1), ImVec2(mouseX, y2), uBlack, 2.f);
            // Arrows
            DRAWLIST->AddLine(ImVec2(mouseX, y1), ImVec2(mouseX - arrowSize, y1 + arrowSize), uBlack, 2.f);
            DRAWLIST->AddLine(ImVec2(mouseX, y1), ImVec2(mouseX + arrowSize, y1 + arrowSize), uBlack, 2.f);
            DRAWLIST->AddLine(ImVec2(mouseX, y2), ImVec2(mouseX - arrowSize, y2 - arrowSize), uBlack, 2.f);
            DRAWLIST->AddLine(ImVec2(mouseX, y2), ImVec2(mouseX + arrowSize, y2 - arrowSize), uBlack, 2.f);
            // Text
            snprintf(tmpStr, sizeof(tmpStr), "{ %s }", getNiceDuration(mouseTimeBestTimeNs - lv.rangeSelStartNs));
            ImVec2 tb = ImGui::CalcTextSize(tmpStr);
            float  x3 = mouseX - 0.5f * tb.x;
            DRAWLIST->AddRectFilled(ImVec2(x3 - 5.f, mouseY - tb.y - 5.f), ImVec2(x3 + tb.x + 5.f, mouseY - 5.f),
                                    IM_COL32(255, 255, 255, 192));
            DRAWLIST->AddText(ImVec2(x3, mouseY - tb.y - 5.f), uBlack, tmpStr);
        }
    }
    // Drag ended: set the selected range view
    else if (isWindowHovered && lv.rangeSelStartNs >= 0) {
        lv.rangeSelStartNs = -1;
    }

    // Display and update the mouse time
    if (mouseTimeBestY >= 0) {
        DRAWLIST->AddLine(ImVec2(winX, mouseTimeBestY), ImVec2(winX + curScrollPosX + winWidth, mouseTimeBestY), uYellow);
    }
    if (newMouseTimeNs >= 0) _mouseTimeNs = newMouseTimeNs;

    ImGui::SetCursorPos(ImVec2(maxOffsetX - winX + curScrollPosX, lv.cachedLogs.size() * fontHeight));
    ImGui::Dummy(ImVec2(0, 0));  // Need this dummy item after setting the cursor position and extending the window
    ImGui::EndChild();
}
