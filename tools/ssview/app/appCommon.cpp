

#include "appCommon.h"

#include "asserted.h"
#include "bs.h"
#include "bsVec.h"
#include "imgui.h"
#include "imgui_internal.h"  // For the DockBuilder API (alpha) + title bar tooltip

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

const char*
getNiceDuration(int64_t ns, int64_t displayRangeNs, int bank)
{
    static char outBuf1[32];
    static char outBuf2[32];
    char*       outBuf = (bank == 0) ? outBuf1 : outBuf2;
    if (displayRangeNs <= 0) displayRangeNs = ns;
    if (displayRangeNs < 1000)
        snprintf(outBuf, sizeof(outBuf1), "%" PRId64 " ns", ns);
    else if (displayRangeNs < 1000000)
        snprintf(outBuf, sizeof(outBuf1), "%.2f µs", 0.001 * ns);
    else if (displayRangeNs < 1000000000)
        snprintf(outBuf, sizeof(outBuf1), "%.2f ms", 0.000001 * ns);
    else
        snprintf(outBuf, sizeof(outBuf1), "%.2f s", 0.000000001 * ns);
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
    bsVec<ImGuiDockNode*> stack;
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
