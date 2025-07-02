
#include "imgui.h"

// Internal
#include "appCommon.h"
#include "main.h"

void
appMain::addTextView(uint32_t id)
{
    _textViews.push_back({});
    _textViews.back().uniqueId = id;
    snprintf(_textViews.back().name, sizeof(_textViews.back().name), "Text");
}

void
appMain::prepareTextData(TextView& lv)
{
    if (!lv.isDataDirty) { return; }
    lv.isDataDirty = false;
    lv.cachedElems.clear();
    lv.maxCategoryLength = 1;
    lv.lengthFontSizeRef = ImGui::CalcTextSize("").y;

    // Build the filtering rules
    std::string                  errorMessage;
    std::vector<sslogread::Rule> rules;
    std::vector<int>             valuePositions;

    // Collect the logs
    if (!_logSession.query(
            rules,
            [this, &lv, &valuePositions](int /*ruleIdx*/, const sslogread::LogStruct& log) {
                // Format the string with arguments (=custom vsnprintf with our argument list, see below)
                char filledFormat[1024];
                sslogread::vsnprintfLog(filledFormat, sizeof(filledFormat), _logSession.getIndexedString(log.formatIdx), log.args,
                                        &_logSession, &valuePositions);

                // Update statistics
                float threadWidth = ImGui::CalcTextSize(_logSession.getIndexedString(log.threadIdx)).x;
                if (threadWidth > lv.maxThreadLength) lv.maxThreadLength = threadWidth;
                float categoryWidth = ImGui::CalcTextSize(_logSession.getIndexedString(log.categoryIdx)).x;
                if (categoryWidth > lv.maxCategoryLength) lv.maxCategoryLength = categoryWidth;

                // Store
                lv.cachedElems.push_back({log.timestampUtcNs - _originUtcNs, log.level, log.threadIdx, log.categoryIdx, filledFormat,
                                          std::move(valuePositions)});

                return true;
            },
            errorMessage)) {
        fprintf(stderr, "Error: %s\n", errorMessage.c_str());
    }
}

void
appMain::drawTexts()
{
    if (_phase != Phase::Active) { return; }
    int itemToRemoveIdx = -1;

    for (int textIdx = 0; textIdx < (int)_textViews.size(); ++textIdx) {
        auto& lv = _textViews[textIdx];

        char tmpStr[256];
        snprintf(tmpStr, sizeof(tmpStr), "%s###%d", lv.name, lv.uniqueId);
        bool isOpen = true;

        if (lv.isNew) {
            lv.isNew = false;
            selectBestDockLocation(false, true);
        }
        if (ImGui::Begin(tmpStr, &isOpen,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs)) {
            drawText(lv);
        }
        if (!isOpen) itemToRemoveIdx = textIdx;
        ImGui::End();
    }

    // Remove profile if needed
    if (itemToRemoveIdx >= 0) {
        _textViews.erase(_textViews.begin() + itemToRemoveIdx);
        dirty();
        // setFullScreenView(-1);
    }
}

void
appMain::drawText(TextView& lv)
{
    constexpr ImU32       levelColors[SSLOG_LEVEL_QTY] = {uWhite, uWhite, uGreen, uYellow, uRed, uRed};
    constexpr const char* levelStr[SSLOG_LEVEL_QTY]    = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL", ""};

    // Configuration bar
    // =================

    float       textBgY              = ImGui::GetWindowPos().y + ImGui::GetCursorPos().y;
    const float padMenuX             = ImGui::GetStyle().FramePadding.x;
    const ImU32 filterBg             = ImColor(ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
    float       charWidth            = ImGui::CalcTextSize("0").x;
    float       lengthFontSizeFactor = ImGui::CalcTextSize("").y / lv.lengthFontSizeRef;
    float       maxLengthLevel       = ImGui::CalcTextSize("Critical").x;
    float       offsetMenuX = ImGui::GetStyle().ItemSpacing.x + padMenuX + maxLengthLevel;  // maxLengthLevel is roughly the right padding

    // Bar background
    DRAWLIST->AddRectFilled(
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetCursorPos().x, textBgY),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x, textBgY + ImGui::GetTextLineHeightWithSpacing()), uGrey48);

    // Display: popup menu with checkboxes  for level, category, buffer content
    float widthMenu = ImGui::CalcTextSize(" Display ").x;
    DRAWLIST->AddRectFilled(
        ImVec2(ImGui::GetWindowPos().x + offsetMenuX - padMenuX, textBgY),
        ImVec2(ImGui::GetWindowPos().x + offsetMenuX + widthMenu + padMenuX, textBgY + ImGui::GetTextLineHeightWithSpacing()), filterBg);
    ImGui::SameLine(offsetMenuX);
    if (ImGui::Selectable(" Display ", false, 0, ImVec2(widthMenu, 0))) ImGui::OpenPopup("Display text menu");
    if (ImGui::BeginPopup("Display text menu", ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("##textViewName", lv.name, sizeof(lv.name));
        if (ImGui::IsItemHovered() && getLastMouseMoveDurationUs() > 300000) {
            ImGui::SetTooltip("Name of the view, displayed in the title bar.");
        }
        ImGui::Checkbox("Level", &lv.doDisplayLevel);
        ImGui::Checkbox("Thread", &lv.doDisplayThread);
        ImGui::Checkbox("Category", &lv.doDisplayCategory);
        ImGui::Checkbox("Buffer content", &lv.doDisplayBufferContent);
        ImGui::EndPopup();
    }
    offsetMenuX += widthMenu;

    ImGui::Separator();

    /* Filtering:
       1a) Start with only 1 filter
       - Combo for min level (no max level)
       - Combo for buffer: Any, only buffer, no buffer
       - TBD: Category, thread, format / Positive & negative
           => On two lines?
       - multiple value fields => Input entry, space separated
       - [+] sign at the end to add more rules

       OR

       1b) At the right of the display, have a "Filter" button which opens a floating window
           => enough space for any config => all fields on the same line (array)

     */

    // Display of the logs
    // ===================

    char tmpStr[256];
    char lastDateStr[256];
    lastDateStr[0] = 0;  // No previous displayed date
    ImGui::BeginChild("text", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    prepareTextData(lv);

    const float fontHeight      = ImGui::GetTextLineHeightWithSpacing();
    const float textPixMargin   = ImGui::GetStyle().ItemSpacing.x;
    const float winX            = ImGui::GetWindowPos().x;
    const float winY            = ImGui::GetWindowPos().y;
    const float curScrollPosX   = ImGui::GetScrollX();
    const float curScrollPosY   = ImGui::GetScrollY();
    const float winWidth        = ImGui::GetWindowContentRegionMax().x;
    const bool  isWindowHovered = ImGui::IsWindowHovered();
    const float mouseX          = ImGui::GetMousePos().x;
    const float mouseY          = ImGui::GetMousePos().y;

    float   maxOffsetX     = winX;
    float   y              = winY;
    float   yEnd           = winY + ImGui::GetWindowSize().y + fontHeight;     // Skip the invisible log part after the window end
    int     logStartIdx    = (curScrollPosY + 0.5 * fontHeight) / fontHeight;  // Skip the invisible log part before the window start
    int64_t dayOriginUtcNs = _dayOriginUtcNs + (_settingsView.useUtc ? -_tzOffsetNs : 0);

    int64_t newMouseTimeNs      = -1;
    int64_t mouseTimeBestTimeNs = -1;
    float   mouseTimeBestY      = -1.;

    // Loop on logs
    for (int logIdx = logStartIdx; logIdx < (int)lv.cachedElems.size() && y < yEnd; ++logIdx) {
        const TextElem& ci = lv.cachedElems[logIdx];

        // Display the date
        float       offsetX = winX + textPixMargin - curScrollPosX;
        const char* timeStr = getFormattedTimeString(ci.timestampUtcNs, _settingsView.timeFormat, dayOriginUtcNs);
        DRAWLIST->AddText(ImVec2(offsetX, y), uWhite, timeStr);
        int changedOffset = 0;
        while (timeStr[changedOffset] && timeStr[changedOffset] == lastDateStr[changedOffset]) ++changedOffset;
        DRAWLIST->AddText(ImVec2(offsetX, y), uGrey128, timeStr, timeStr + changedOffset);
        snprintf(lastDateStr, sizeof(lastDateStr), "%s", timeStr);
        offsetX += charWidth * (float)getFormattedTimeStringCharQty(_settingsView.timeFormat);

        // Display the level
        if (lv.doDisplayLevel && (uint32_t)ci.level < SSLOG_LEVEL_QTY) {
            DRAWLIST->AddText(ImVec2(offsetX, y), levelColors[(int)ci.level], levelStr[(int)ci.level]);
            offsetX += charWidth * 4 + maxLengthLevel;
        }

        // Display the thread
        if (lv.doDisplayThread) {
            ImU32 color =
                _colorPalette[((14695981039346656037ULL + ci.threadIdx + _settingsView.colorSeed) * 16777619ULL) % _colorPalette.size()];
            DRAWLIST->AddText(ImVec2(offsetX, y), color, _logSession.getIndexedString(ci.threadIdx));
            offsetX += charWidth * 4 + lv.maxThreadLength * lengthFontSizeFactor;
        }

        // Display the category
        if (lv.doDisplayCategory) {
            ImU32 color =
                _colorPalette[((14695981039346656037ULL + ci.categoryIdx + _settingsView.colorSeed) * 16777619ULL) % _colorPalette.size()];
            DRAWLIST->AddText(ImVec2(offsetX, y), color, _logSession.getIndexedString(ci.categoryIdx));
            offsetX += charWidth * 4 + lv.maxCategoryLength * lengthFontSizeFactor;
        }

        // Display the message
        DRAWLIST->AddText(ImVec2(offsetX, y), uWhite, ci.message.c_str());

        // Override in color the argument values and names
        asserted((ci.valuePositions.size() & 1) == 0, "The value position array must be even", ci.valuePositions.size());
        for (uint32_t i = 0; i < ci.valuePositions.size(); i += 2) {
            // Find positions
            const char* sStart     = ci.message.c_str();
            const char* valueStart = sStart + ci.valuePositions[i];
            const char* valueEnd   = sStart + ci.valuePositions[i + 1];
            const char* nameEnd    = bsMax(sStart, valueStart - 1);
            while (nameEnd > sStart && (*nameEnd == '_' || *nameEnd == '=' || *nameEnd == ':' || *nameEnd == '-')) --nameEnd;
            ++nameEnd;
            const char* nameStart = nameEnd;
            while (nameStart > sStart && *nameStart != ' ') --nameStart;
            ++nameStart;

            // Write in color
            float redrawOffset = ImGui::CalcTextSize(sStart, valueStart).x;
            DRAWLIST->AddText(ImVec2(offsetX + redrawOffset, y), uCyan, valueStart, valueEnd);

            redrawOffset = ImGui::CalcTextSize(sStart, nameStart).x;
            DRAWLIST->AddText(ImVec2(offsetX + redrawOffset, y), uYellow, nameStart, nameEnd);
        }
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
    if ((isWindowHovered || lv.rangeSelStartNs >= 0) && ImGui::IsMouseDragging(1)) {
        // Start a range selection
        if (lv.rangeSelStartNs < 0 && mouseTimeBestTimeNs >= 0) {
            lv.rangeSelStartNs = mouseTimeBestTimeNs;
            lv.rangeSelStartY  = mouseTimeBestY;
        }

        // Drag on-going: display the selection box with transparency and range
        if (lv.rangeSelStartNs >= 0) {
            float           x         = winX + textPixMargin - curScrollPosX;
            float           y1        = lv.rangeSelStartY - ((lv.rangeSelStartNs < mouseTimeBestTimeNs) ? fontHeight : 0);
            float           y2        = mouseTimeBestY - ((lv.rangeSelStartNs < mouseTimeBestTimeNs) ? 0 : fontHeight);
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
            snprintf(tmpStr, sizeof(tmpStr), "{ %s }", getNiceDuration(bsAbs(mouseTimeBestTimeNs - lv.rangeSelStartNs)));
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

    ImGui::SetCursorPos(ImVec2(maxOffsetX - winX + curScrollPosX, (lv.cachedElems.size() + 1) * fontHeight));
    ImGui::Dummy(ImVec2(0, 0));  // Need this dummy item after setting the cursor position and extending the window
    ImGui::EndChild();
}
