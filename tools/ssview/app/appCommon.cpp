

#include "appCommon.h"

#include <vector>

#include "asserted.h"
#include "bs.h"
#include "imgui.h"
#include "imgui_internal.h"  // For the DockBuilder API (alpha) + title bar tooltip

int
getFormattedTimeStringCharQty(TimeFormat timeFormat)
{
    // 2 char of margin
    constexpr int timeStrCharQtyArray[(int)TimeFormat::Qty] = {17 + 2, 13 + 2, 21 + 2, 17 + 2};
    int           timeFormatInt                             = (int)timeFormat;
    asserted(timeFormatInt < (int)TimeFormat::Qty, timeFormatInt, (int)TimeFormat::Qty);
    return timeStrCharQtyArray[timeFormatInt];
}

const char*
getFormattedTimeString(int64_t ns, TimeFormat timeFormat, int64_t dayOriginUtcNs)
{
    static char outBuf[64];
    if (timeFormat == TimeFormat::Nanosecond) {
        snprintf(outBuf, sizeof(outBuf), "%d.%03d_%03d_%03ds", (int)(ns / 1000000000LL), (int)((ns / 1000000LL) % 1000),
                 (int)((ns / 1000LL) % 1000), (int)(ns % 1000));
    } else if (timeFormat == TimeFormat::Microsecond) {
        snprintf(outBuf, sizeof(outBuf), "%d.%03d_%03d", (int)(ns / 1000000000LL), (int)((ns / 1000000LL) % 1000),
                 (int)((ns / 1000LL) % 1000));
    } else if (timeFormat == TimeFormat::HhMmSsNanosecond) {
        ns += dayOriginUtcNs;
        if (ns < 0) ns += 24 * 3600000000000LL;  // + 24h, to avoid negative number due to timezone
        snprintf(outBuf, sizeof(outBuf), "%02d:%02d:%02d.%03d_%03d_%03d", (int)(ns / 3600000000000LL) % 24,
                 (int)((ns / 60000000000LL) % 60), (int)((ns / 1000000000LL) % 60), (int)((ns / 1000000LL) % 1000),
                 (int)((ns / 1000LL) % 1000), (int)(ns % 1000));
    } else if (timeFormat == TimeFormat::HhMmSsMicrosecond) {
        ns += dayOriginUtcNs;
        if (ns < 0) ns += 24 * 3600000000000LL;
        snprintf(outBuf, sizeof(outBuf), "%02d:%02d:%02d.%03d_%03d", (int)(ns / 3600000000000LL), (int)((ns / 60000000000LL) % 60),
                 (int)((ns / 1000000000LL) % 60), (int)((ns / 1000000LL) % 1000), (int)((ns / 1000LL) % 1000));
    } else {
        asserted(false, "Unhandled time format", (int)timeFormat);
    }

    return outBuf;
}

const char*
getNiceDuration(int64_t ns, int64_t displayRangeNs)
{
    static char outBuf[32];
    if (displayRangeNs <= 0) displayRangeNs = ns;
    if (displayRangeNs < 1000)
        snprintf(outBuf, sizeof(outBuf), "%" PRId64 " ns", ns);
    else if (displayRangeNs < 1000000)
        snprintf(outBuf, sizeof(outBuf), "%.2f µs", 0.001 * ns);
    else if (displayRangeNs < 1000000000)
        snprintf(outBuf, sizeof(outBuf), "%.2f ms", 0.000001 * ns);
    else
        snprintf(outBuf, sizeof(outBuf), "%.2f s", 0.000000001 * ns);
    return outBuf;
}

void
selectBestDockLocation(bool bigWidth, bool bigHeight)
{
    // Get root node and prepare recursive parsing
    ImGuiID        mainDockspaceId = ImGui::GetID("MainDockSpace");
    ImGuiDockNode* root            = ImGui::DockBuilderGetNode(mainDockspaceId);
    asserted(root);
    struct ClassDockId {
        ImGuiID id        = 0;
        float   criterion = 0.;
    };
    ClassDockId cds[4];  // 0 = biggest area, 1=highest, 2=widest, 3=smallest area

    // Parse the layout tree
    std::vector<ImGuiDockNode*> stack;
    stack.reserve(32);
    stack.push_back(root);
    while (!stack.empty()) {
        ImGuiDockNode* node = stack.back();
        stack.pop_back();
        if (node->IsLeafNode()) {
            // Categorize the node
            ImVec2& s         = node->SizeRef;
            float   criterion = 0.f;
            for (int classKind = 0; classKind < 4; ++classKind) {
                switch (classKind) {
                    case 0:
                        criterion = s[0] * s[1] / sqrt(bsMax(s[0] / s[1], s[1] / s[0]));
                        break;  // Max area and favoring square shape
                    case 1:
                        criterion = s[1] / sqrt(s[0]);
                        break;  // Max height and favoring thin shape
                    case 2:
                        criterion = s[0] / sqrt(s[1]);
                        break;  // Max width and favoring thick shape
                    case 3:
                        criterion = 1.f / (s[0] * s[1]);
                        break;  // Smallest area
                }
                criterion *=
                    1.f - ((node->TabBar) ? 0.001f * node->TabBar->Tabs.size() : 0.f);  // Slightly favorize node with least views inside
                if (cds[classKind].id == 0 || cds[classKind].criterion < criterion) cds[classKind] = {node->ID, criterion};
            }
        } else {
            // Propagate
            stack.push_back(node->ChildNodes[0]);
            stack.push_back(node->ChildNodes[1]);
        }
    }

    // Assign the next ImGui window to this dockinbg location
    int classIdx = (bigWidth ? 0 : 1) + (bigHeight ? 0 : 2);
    ImGui::SetNextWindowDockID(cds[classIdx].id);
}
