
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
appMain::prepareTextData(TextView& tv)
{
    if (!tv.isDataDirty) { return; }
    tv.isDataDirty = false;
    tv.cachedElems.clear();
    tv.maxCategoryLength = 1;
    tv.lengthFontSizeRef = ImGui::CalcTextSize("").y;

    // Build the filtering rules
    std::string                  errorMessage;
    std::vector<sslogread::Rule> rules;
    std::vector<int>             valuePositions;

    // Collect the logs
    if (!_logSession.query(
            rules,
            [this, &tv, &valuePositions](int /*ruleIdx*/, const sslogread::LogStruct& log) {
                // Format the string with arguments (=custom vsnprintf with our argument list, see below)
                char filledFormat[1024];
                sslogread::vsnprintfLog(filledFormat, sizeof(filledFormat), _logSession.getIndexedString(log.formatIdx), log.args,
                                        &_logSession, &valuePositions);

                // Update statistics
                float threadWidth = ImGui::CalcTextSize(_logSession.getIndexedString(log.threadIdx)).x;
                if (threadWidth > tv.maxThreadLength) tv.maxThreadLength = threadWidth;
                float categoryWidth = ImGui::CalcTextSize(_logSession.getIndexedString(log.categoryIdx)).x;
                if (categoryWidth > tv.maxCategoryLength) tv.maxCategoryLength = categoryWidth;

                // Store
                tv.cachedElems.push_back({log.timestampUtcNs - _originUtcNs, log.level, log.threadIdx, log.categoryIdx, filledFormat,
                                          std::move(valuePositions), log.buffer});

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
        auto& tv = _textViews[textIdx];

        char tmpStr[256];
        snprintf(tmpStr, sizeof(tmpStr), "%s###%d", tv.name, tv.uniqueId);
        bool isOpen = true;

        if (tv.isNew) {
            tv.isNew = false;
            selectBestDockLocation(false, true);
        }
        if (ImGui::Begin(tmpStr, &isOpen,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs)) {
            drawText(tv);
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
appMain::drawText(TextView& tv)
{
    constexpr ImU32       levelColors[SSLOG_LEVEL_QTY] = {uWhite, uWhite, uGreen, uYellow, uRed, uRed};
    constexpr const char* levelStr[SSLOG_LEVEL_QTY]    = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL", ""};

    // Configuration bar
    // =================

    const float winWidth             = ImGui::GetWindowContentRegionMax().x;
    const float winHeight            = ImGui::GetWindowContentRegionMax().y;
    float       textBgY              = ImGui::GetWindowPos().y + ImGui::GetCursorPos().y;
    const float padMenuX             = ImGui::GetStyle().FramePadding.x;
    const ImU32 filterBg             = ImColor(ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
    float       charWidth            = ImGui::CalcTextSize("0").x;
    float       lengthFontSizeFactor = ImGui::CalcTextSize("").y / tv.lengthFontSizeRef;
    float       maxLengthLevel       = ImGui::CalcTextSize("Critical").x;
    float       offsetMenuX = ImGui::GetStyle().ItemSpacing.x + padMenuX + maxLengthLevel;  // maxLengthLevel is roughly the right padding

    // Bar background
    DRAWLIST->AddRectFilled(
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetCursorPos().x, textBgY),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x, textBgY + ImGui::GetTextLineHeightWithSpacing()), uGrey48);

    // Display: popup menu with checkboxes
    float widthMenu = ImGui::CalcTextSize(" Display ").x;
    DRAWLIST->AddRectFilled(
        ImVec2(ImGui::GetWindowPos().x + offsetMenuX - padMenuX, textBgY),
        ImVec2(ImGui::GetWindowPos().x + offsetMenuX + widthMenu + padMenuX, textBgY + ImGui::GetTextLineHeightWithSpacing()), filterBg);
    ImGui::SameLine(offsetMenuX);
    if (ImGui::Selectable(" Display ", false, 0, ImVec2(widthMenu, 0))) ImGui::OpenPopup("Display text menu");
    if (ImGui::BeginPopup("Display text menu", ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("##textViewName", tv.name, sizeof(tv.name));
        if (ImGui::IsItemHovered() && getLastMouseMoveDurationUs() > 300000) {
            ImGui::SetTooltip("Name of the view, displayed in the title bar.");
        }
        ImGui::Checkbox("Level", &tv.doDisplayLevel);
        ImGui::Checkbox("Thread", &tv.doDisplayThread);
        ImGui::Checkbox("Category", &tv.doDisplayCategory);
        ImGui::Checkbox("Buffer content", &tv.doDisplayBufferContent);
        ImGui::EndPopup();
    }
    offsetMenuX += widthMenu + 8.f * padMenuX;

    // Filter: popup menu with complex API
    widthMenu = ImGui::CalcTextSize(" Filter ").x;
    DRAWLIST->AddRectFilled(
        ImVec2(ImGui::GetWindowPos().x + offsetMenuX - padMenuX, textBgY),
        ImVec2(ImGui::GetWindowPos().x + offsetMenuX + widthMenu + padMenuX, textBgY + ImGui::GetTextLineHeightWithSpacing()), filterBg);
    ImGui::SameLine(offsetMenuX);
    if (ImGui::Selectable(" Filter ", false, 0, ImVec2(widthMenu, 0))) ImGui::OpenPopup("Filter text menu");
    if (ImGui::BeginPopup("Filter text menu", ImGuiWindowFlags_AlwaysAutoResize)) {
        // @TODO
        ImGui::Checkbox("Buffer content", &tv.doDisplayBufferContent);

        ImGui::EndPopup();
    }
    offsetMenuX += widthMenu + 8.f * padMenuX;

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

    ImGui::Separator();

    // Display of the logs
    // ===================

    char tmpStr[256];
    char lastDateStr[256];
    lastDateStr[0] = 0;  // No previous displayed date
    ImGui::BeginChild("text", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    prepareTextData(tv);

    const float fontHeight      = ImGui::GetTextLineHeightWithSpacing();
    const float textPixMargin   = ImGui::GetStyle().ItemSpacing.x;
    const float winX            = ImGui::GetWindowPos().x;
    const float winY            = ImGui::GetWindowPos().y;
    const float curScrollPosX   = ImGui::GetScrollX();
    const float curScrollPosY   = ImGui::GetScrollY();
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
    for (int logIdx = logStartIdx; logIdx < (int)tv.cachedElems.size() && y < yEnd; ++logIdx) {
        const TextElem& ci = tv.cachedElems[logIdx];

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
        if (tv.doDisplayLevel && (uint32_t)ci.level < SSLOG_LEVEL_QTY) {
            DRAWLIST->AddText(ImVec2(offsetX, y), levelColors[(int)ci.level], levelStr[(int)ci.level]);
            offsetX += charWidth * 4 + maxLengthLevel;
        }

        // Display the thread
        if (tv.doDisplayThread) {
            ImU32 color =
                _colorPalette[((14695981039346656037ULL + ci.threadIdx + _settingsView.colorSeed) * 16777619ULL) % _colorPalette.size()];
            DRAWLIST->AddText(ImVec2(offsetX, y), color, _logSession.getIndexedString(ci.threadIdx));
            offsetX += charWidth * 4 + tv.maxThreadLength * lengthFontSizeFactor;
        }

        // Display the category
        if (tv.doDisplayCategory) {
            ImU32 color =
                _colorPalette[((14695981039346656037ULL + ci.categoryIdx + _settingsView.colorSeed) * 16777619ULL) % _colorPalette.size()];
            DRAWLIST->AddText(ImVec2(offsetX, y), color, _logSession.getIndexedString(ci.categoryIdx));
            offsetX += charWidth * 4 + tv.maxCategoryLength * lengthFontSizeFactor;
        }

        // Display the message
        DRAWLIST->AddText(ImVec2(offsetX, y), uWhite, ci.message.c_str());
        float messageLength = ImGui::CalcTextSize(ci.message.c_str()).x;
        if (!ci.buffer.empty() && !tv.doDisplayBufferContent) {
            snprintf(tmpStr, sizeof(tmpStr), " (+ buffer [%u])", (uint32_t)ci.buffer.size());
            DRAWLIST->AddText(ImVec2(offsetX + messageLength, y), uCyan, tmpStr);
            messageLength += ImGui::CalcTextSize(tmpStr).x;
        }

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

        // Display the buffer, if any
        if (!ci.buffer.empty() && tv.doDisplayBufferContent) {
            char* p    = tmpStr;  // Big enough for a full line
            char* pEnd = tmpStr + sizeof(tmpStr) - 1;
            for (uint32_t i = 0; i < (uint32_t)ci.buffer.size(); ++i) {
                if ((i % 32) == 0) { p += snprintf(p, pEnd - p, "        %04x   ", i); }
                p += snprintf(p, pEnd - p, "%02X ", ci.buffer[i]);
                if (((i + 1) % 32) == 0) {
                    // Draw the finished line
                    y += fontHeight;
                    DRAWLIST->AddText(ImVec2(offsetX, y), uCyan, tmpStr);
                    // New line
                    p = tmpStr;
                } else if (((i + 1) % 16) == 0) {
                    p += snprintf(p, pEnd - p, " ");
                }
            }
            // Flush
            if (p != tmpStr) {
                y += fontHeight;
                DRAWLIST->AddText(ImVec2(offsetX, y), uCyan, tmpStr);
            }
        }
        offsetX += messageLength;

        if (isWindowHovered && mouseY > y) newMouseTimeNs = ci.timestampUtcNs;
        if (_mouseTimeNs >= (int64_t)ci.timestampUtcNs && (int64_t)ci.timestampUtcNs > mouseTimeBestTimeNs) {
            mouseTimeBestTimeNs = ci.timestampUtcNs;
            mouseTimeBestY      = y + fontHeight;
        }

        if (offsetX > maxOffsetX) maxOffsetX = offsetX;
        y += fontHeight;
    }

    // Drag with middle button
    if ((isWindowHovered || tv.rangeSelStartNs >= 0) && ImGui::IsMouseDragging(1)) {
        // Start a range selection
        if (tv.rangeSelStartNs < 0 && mouseTimeBestTimeNs >= 0) {
            tv.rangeSelStartNs = mouseTimeBestTimeNs;
            tv.rangeSelStartY  = mouseTimeBestY;
        }

        // Drag on-going: display the selection box with transparency and range
        if (tv.rangeSelStartNs >= 0) {
            float           x         = winX + textPixMargin - curScrollPosX;
            float           y1        = tv.rangeSelStartY - ((tv.rangeSelStartNs < mouseTimeBestTimeNs) ? fontHeight : 0);
            float           y2        = mouseTimeBestY - ((tv.rangeSelStartNs < mouseTimeBestTimeNs) ? 0 : fontHeight);
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
            snprintf(tmpStr, sizeof(tmpStr), "{ %s }", getNiceDuration(bsAbs(mouseTimeBestTimeNs - tv.rangeSelStartNs)));
            ImVec2 tb = ImGui::CalcTextSize(tmpStr);
            float  x3 = mouseX - 0.5f * tb.x;
            DRAWLIST->AddRectFilled(ImVec2(x3 - 5.f, mouseY - tb.y - 5.f), ImVec2(x3 + tb.x + 5.f, mouseY - 5.f),
                                    IM_COL32(255, 255, 255, 192));
            DRAWLIST->AddText(ImVec2(x3, mouseY - tb.y - 5.f), uBlack, tmpStr);
        }
    }
    // Drag ended: set the selected range view
    else if (isWindowHovered && tv.rangeSelStartNs >= 0) {
        tv.rangeSelStartNs = -1;
    }

    // Display and update the mouse time
    if (mouseTimeBestY >= 0) {
        DRAWLIST->AddLine(ImVec2(winX, mouseTimeBestY), ImVec2(winX + curScrollPosX + winWidth, mouseTimeBestY), uYellow);
    }
    if (newMouseTimeNs >= 0) _mouseTimeNs = newMouseTimeNs;

    ImGui::SetCursorPos(ImVec2(maxOffsetX - winX + curScrollPosX, tv.cachedElems.size() * fontHeight + winHeight));
    ImGui::Dummy(ImVec2(0, 0));  // Need this dummy item after setting the cursor position and extending the window
    ImGui::EndChild();
}
