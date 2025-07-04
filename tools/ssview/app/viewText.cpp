
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

    // Collect the logs
    std::string      errorMessage;
    std::vector<int> valuePositions;
    if (!_logSession.query(
            tv.rules,
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
                tv.cachedElems.push_back({log.timestampUtcNs - _originUtcNs, log.level, log.threadIdx, log.categoryIdx, log.formatIdx,
                                          filledFormat, std::move(valuePositions), log.buffer});

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
    }
}

void
appMain::drawTextFilterConfig(TextView& tv)
{
    char  tmpStr[256];
    int   ruleToRemoveIdx = -1;
    int   ruleToAddIdx    = -1;
    float charWidth       = ImGui::CalcTextSize("0").x;
    float maxLengthLevel  = charWidth * 8;

#define TEXT_TOOLTIP(message) \
    if (ImGui::IsItemHovered() && getLastMouseMoveDurationUs() > 300000) { ImGui::SetTooltip(message); }

    ImGui::Text(
        "All criteria must match inside a rule (logical AND inside a rule).         "
        "One matching rule is enough (logical OR between rules).         "
        "Hover on criteria names to get explanations and examples");
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // Loop on rules
    for (uint32_t ruleIdx = 0; ruleIdx < _rulesUnderWork.size(); ++ruleIdx) {
        sslogread::Rule& rule = _rulesUnderWork[ruleIdx];
        ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, charWidth * 0.25f);
        ImGui::PushItemWidth(maxLengthLevel * 2.0);
        ImGui::PushID(ruleIdx);

        // Level
        int levelMin = (int)rule.levelMin;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Min level:");
        TEXT_TOOLTIP("Minimum log level (included)");
        ImGui::SameLine();
        if (ImGui::Combo("##Min level", &levelMin, "Trace\0Debug\0Info\0Warning\0Error\0Critical\0\0")) {
            rule.levelMin = sslog::Level{levelMin};
            if ((int)rule.levelMax < levelMin) { rule.levelMax = sslog::Level{levelMin}; }
        }
        ImGui::SameLine(0., charWidth * 2.f);
        int levelMax = (int)rule.levelMax;
        ImGui::Text("Max level:");
        TEXT_TOOLTIP("Maximum log level (included)");
        ImGui::SameLine();
        if (ImGui::Combo("##Max level", &levelMax, "Trace\0Debug\0Info\0Warning\0Error\0Critical\0\0")) {
            rule.levelMax = sslog::Level{levelMax};
            if ((int)rule.levelMin > levelMax) { rule.levelMin = sslog::Level{levelMax}; }
        }

        // Category
        ImGui::SameLine(0., charWidth * 2.f);
        snprintf(tmpStr, sizeof(tmpStr), "%s", rule.category.c_str());
        float categoryPosX = ImGui::GetCursorPos().x;
        ImGui::Text("Category:");
        TEXT_TOOLTIP(
            "Filter-in category pattern which shall match exactly, wildcard accepted.\nOnly one pattern.\nEx: 'Engine/config' or "
            "'pipe*/graphic*'");
        ImGui::SameLine(categoryPosX + ImGui::CalcTextSize("No category:").x + charWidth * 0.25f);
        if (ImGui::InputText("##Category", tmpStr, sizeof(tmpStr))) { rule.category = tmpStr; }

        // Thread
        ImGui::SameLine(0., charWidth * 2.f);
        snprintf(tmpStr, sizeof(tmpStr), "%s", rule.thread.c_str());
        float threadPosX = ImGui::GetCursorPos().x;
        ImGui::Text("Thread:");
        TEXT_TOOLTIP(
            "Filter-in thread pattern which shall match exactly, wildcard accepted.\nOnly one pattern.\nEx: 'main' or '*/worker*'");
        ImGui::SameLine(threadPosX + ImGui::CalcTextSize("No thread:").x + charWidth * 0.25f);
        if (ImGui::InputText("##Thread", tmpStr, sizeof(tmpStr))) { rule.thread = tmpStr; }

        // Format
        ImGui::SameLine(0., charWidth * 2.f);
        snprintf(tmpStr, sizeof(tmpStr), "%s", rule.format.c_str());
        float formatPosX = ImGui::GetCursorPos().x;
        ImGui::Text("Format:");
        TEXT_TOOLTIP("Filter-in format pattern which shall match exactly, wildcard accepted.\nOnly one pattern.\nEx: '* independent of*'");
        ImGui::SameLine(formatPosX + ImGui::CalcTextSize("No format:").x + charWidth * 0.25f);
        if (ImGui::InputText("##Format", tmpStr, sizeof(tmpStr))) { rule.format = tmpStr; }

        // Arguments
        ImGui::SameLine(0., charWidth * 2.f);
        tmpStr[0]  = 0;
        char* p    = tmpStr;
        char* pEnd = tmpStr + sizeof(tmpStr);
        for (uint32_t i = 0; i < rule.arguments.size(); ++i) {
            if (i > 0) { p += snprintf(p, pEnd - p, " "); }
            p += snprintf(p, pEnd - p, "%s", rule.arguments[i].c_str());
            if (p > pEnd) { p = pEnd; }
        }
        ImGui::Text("Arguments:");
        TEXT_TOOLTIP(
            "Space separated argument names (multiple accepted), optionally with a value expression.\nNames and values shall match "
            "exactly (no wildcard).\nEx: 'health index=14 size<=1500 name=Gert");
        ImGui::SameLine();
        if (ImGui::InputText("##Arguments", tmpStr, sizeof(tmpStr))) {
            rule.arguments.clear();
            char* p = tmpStr;
            while (*p) {
                while (*p == ' ') ++p;
                pEnd = p;
                while (*pEnd && *pEnd != ' ') ++pEnd;
                if (p != pEnd) { rule.arguments.push_back(std::string(p, pEnd)); }
                p = pEnd;
            }
        }

        // Buffer size
        ImGui::SameLine(0., charWidth * 2.f);
        snprintf(tmpStr, sizeof(tmpStr), "%s", rule.format.c_str());
        ImGui::Text("Buffer size range:");
        TEXT_TOOLTIP("Accepted range on the buffer size (bounds included).\nDouble click for entering a number.");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(charWidth * 8.f);
        if (ImGui::DragInt("##Min buffer size", (int*)&rule.bufferSizeMin, 1.0f, 0, 65535, "%d")) {
            if (*(int*)&rule.bufferSizeMin < 0) { rule.bufferSizeMin = 0; }
            if (rule.bufferSizeMin > rule.bufferSizeMax) { rule.bufferSizeMax = rule.bufferSizeMin; }
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(charWidth * 8.f);
        if (ImGui::DragInt("##Max buffer size", (int*)&rule.bufferSizeMax, 1.0f, 0, 65535, "%d")) {
            if (rule.bufferSizeMax < rule.bufferSizeMin) { rule.bufferSizeMin = rule.bufferSizeMax; }
        }

        // "Remove rule" button
        ImGui::SameLine(0., charWidth * 8.f);
        float ruleChangePosX = ImGui::GetCursorPos().x;
        if (_rulesUnderWork.size() > 1) {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 64, 64, 255));
            if (ImGui::SmallButton("Remove")) { ruleToRemoveIdx = ruleIdx; }
            ImGui::PopStyleColor();
        } else {
            ImGui::NewLine();  // Cancels the "SameLine()
        }

        ImGui::NewLine();

        // No Category
        ImGui::SameLine(categoryPosX);
        snprintf(tmpStr, sizeof(tmpStr), "%s", rule.noCategory.c_str());
        ImGui::Text("No category:");
        TEXT_TOOLTIP(
            "Filter-OUT category pattern which shall match exactly, wildcard accepted.\nOnly one pattern.\nEx: 'Engine/config' or "
            "'pipe*/graphic*'");
        ImGui::SameLine();
        if (ImGui::InputText("##NoCategory", tmpStr, sizeof(tmpStr))) { rule.noCategory = tmpStr; }

        // No Thread
        ImGui::SameLine(threadPosX);
        snprintf(tmpStr, sizeof(tmpStr), "%s", rule.noThread.c_str());
        ImGui::Text("No thread:");
        TEXT_TOOLTIP(
            "Filter-OUT thread pattern which shall match exactly, wildcard accepted.\nOnly one pattern.\nEx: 'main' or '*/worker*'");
        ImGui::SameLine();
        if (ImGui::InputText("##NoThread", tmpStr, sizeof(tmpStr))) { rule.noThread = tmpStr; }

        // No Format
        ImGui::SameLine(formatPosX);
        snprintf(tmpStr, sizeof(tmpStr), "%s", rule.noFormat.c_str());
        ImGui::Text("No format:");
        TEXT_TOOLTIP("Filter-OUT format pattern which shall match exactly, wildcard accepted.\nOnly one pattern.\nEx: '* independent of*'");
        ImGui::SameLine();
        if (ImGui::InputText("##NoFormat", tmpStr, sizeof(tmpStr))) { rule.noFormat = tmpStr; }

        // "Add rule" button
        if (_rulesUnderWork.size() < 8) {
            ImGui::SameLine(ruleChangePosX);
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(64, 128, 64, 255));
            if (ImGui::SmallButton("Add")) { ruleToAddIdx = ruleIdx; }
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();
    }

    // Change in rule quantity (both are exclusive at the same moment)
    if (ruleToRemoveIdx >= 0) { _rulesUnderWork.erase(_rulesUnderWork.begin() + ruleToRemoveIdx); }
    if (ruleToAddIdx >= 0) { _rulesUnderWork.insert(_rulesUnderWork.begin() + ruleToAddIdx + 1, sslogread::Rule{}); }

    // Footer of the filter configuration
    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 3.f * ImGui::CalcTextSize("Cancel").x);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 64, 64, 255));
    if (ImGui::SmallButton("Cancel")) { ImGui::CloseCurrentPopup(); }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(64, 128, 64, 255));
    if (ImGui::SmallButton("OK")) {
        tv.rules       = _rulesUnderWork;
        tv.isDataDirty = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor();
}

void
appMain::drawText(TextView& tv)
{
    constexpr ImU32       levelColors[SSLOG_LEVEL_QTY] = {uWhite, uWhite, uGreen, uYellow, uRed, uRed};
    constexpr const char* levelStr[SSLOG_LEVEL_QTY]    = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL", ""};
    char                  tmpStr[256];

    // Ensure at least one rule (for filter configuration)
    if (tv.rules.empty()) { tv.rules.push_back({}); }

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
    if (ImGui::Selectable(" Display ", false, 0, ImVec2(widthMenu, 0))) { ImGui::OpenPopup("Display text menu"); }
    if (ImGui::BeginPopup("Display text menu", ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("##textViewName", tv.name, sizeof(tv.name));
        TEXT_TOOLTIP("Name of the view, displayed in the title bar.");
        ImGui::Checkbox("Level", &tv.doDisplayLevel);
        ImGui::Checkbox("Thread", &tv.doDisplayThread);
        ImGui::Checkbox("Category", &tv.doDisplayCategory);
        ImGui::Checkbox("Buffer content", &tv.doDisplayBufferContent);
        ImGui::EndPopup();
    }
    offsetMenuX += widthMenu + 8.f * padMenuX;

    // Filter: modal popup menu
    widthMenu = ImGui::CalcTextSize(" Filter ").x;
    DRAWLIST->AddRectFilled(
        ImVec2(ImGui::GetWindowPos().x + offsetMenuX - padMenuX, textBgY),
        ImVec2(ImGui::GetWindowPos().x + offsetMenuX + widthMenu + padMenuX, textBgY + ImGui::GetTextLineHeightWithSpacing()), filterBg);
    ImGui::SameLine(offsetMenuX);
    if (ImGui::Selectable(" Filter ", false, 0, ImVec2(widthMenu, 0))) {
        ImGui::OpenPopup("Log filtering rules");
        _rulesUnderWork = tv.rules;  // Copy current rules as a start point of the edition
    }
    offsetMenuX += widthMenu + 8.f * padMenuX;

    if (ImGui::BeginPopupModal("Log filtering rules", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Draw the configuration UI
        drawTextFilterConfig(tv);
        ImGui::EndPopup();
    }

    // Display of the logs
    // ===================

    char lastDateStr[256];
    lastDateStr[0] = 0;  // No previous displayed date
    ImGui::Separator();
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
    int64_t dayOriginUtcNs = _dayOriginUtcNs + (_settingsView.useLocalTime ? 0 : -_tzOffsetNs);

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
        for (uint32_t argIdx = 0; argIdx < ci.valuePositions.size() / 2; ++argIdx) {
            // Find positions
            const char* sStart     = ci.message.c_str();
            const char* valueStart = sStart + ci.valuePositions[2 * argIdx + 0];
            const char* valueEnd   = sStart + ci.valuePositions[2 * argIdx + 1];
            const char* nameEnd    = bsMax(sStart, valueStart - 1);
            while (nameEnd > sStart && (*nameEnd == '_' || *nameEnd == '=' || *nameEnd == ':' || *nameEnd == '-')) --nameEnd;
            ++nameEnd;
            const char* nameStart = nameEnd;
            while (nameStart > sStart && *nameStart != ' ') --nameStart;
            ++nameStart;

            // Write in color
            ImVec2 nameValue = ImVec2(offsetX + ImGui::CalcTextSize(sStart, valueStart).x, y);
            DRAWLIST->AddText(nameValue, uCyan, valueStart, valueEnd);

            ImVec2 namePos = ImVec2(offsetX + ImGui::CalcTextSize(sStart, nameStart).x, y);
            DRAWLIST->AddText(namePos, uYellow, nameStart, nameEnd);

            // Handle hovering
            if (ImGui::IsMouseHoveringRect(namePos, ImVec2(nameValue.x + ImGui::CalcTextSize(valueStart, valueEnd).x, y + fontHeight)) &&
                ImGui::IsMouseReleased(2)) {
                ImGui::OpenPopup("Plot popup menu");
                _popupMenuFormatIndex = ci.formatIdx;
                _popupMenuArgIndex    = argIdx;
            }
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

    // Double click: synchronize all text windows to this date
    if (isWindowHovered && ImGui::IsMouseDoubleClicked(0) && mouseTimeBestTimeNs >= 0) {
        for (auto& t : _textViews) { t.targetScrollDateNs = mouseTimeBestTimeNs; }
    }
    if (tv.targetScrollDateNs >= 0) {
        for (int logIdx = 0; logIdx < (int)tv.cachedElems.size(); ++logIdx) {
            const TextElem& ci = tv.cachedElems[logIdx];
            if (ci.timestampUtcNs > (uint64_t)tv.targetScrollDateNs) {
                ImGui::SetScrollY(bsMax(0.f, fontHeight * (logIdx - 1)));
                break;
            }
        }
        tv.targetScrollDateNs = -1;
    }

    // Display and update the mouse time
    if (mouseTimeBestY >= 0) {
        DRAWLIST->AddLine(ImVec2(winX, mouseTimeBestY), ImVec2(winX + curScrollPosX + winWidth, mouseTimeBestY), uYellow);
    }
    if (newMouseTimeNs >= 0) _mouseTimeNs = newMouseTimeNs;

    // Handle the popup plot menu
    if (ImGui::BeginPopup("Plot popup menu", ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginMenu("Plot menu")) {
            // @LATER: extend the menu by offering adding a curve to an existing plot with compatible unit
            if (ImGui::MenuItem("Simple plot")) { addPlotView(getId(), tv.rules, _popupMenuFormatIndex, _popupMenuArgIndex); }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::SetCursorPos(ImVec2(maxOffsetX - winX + curScrollPosX, tv.cachedElems.size() * fontHeight + winHeight));
    ImGui::Dummy(ImVec2(0, 0));  // Need this dummy item after setting the cursor position and extending the window
    ImGui::EndChild();
}
