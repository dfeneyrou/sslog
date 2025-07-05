
#include "imgui.h"
#include "implot.h"

// Internal
#include "appCommon.h"
#include "main.h"

void
appMain::addPlotView(uint32_t id, const std::vector<sslogread::Rule>& rules, int formatIdx, int argIdx)
{
    _plotViews.push_back({});
    PlotView& pv = _plotViews.back();
    pv.uniqueId  = id;
    pv.rules     = rules;
    pv.formatIdx = formatIdx;
    pv.argIdx    = argIdx;
    snprintf(_plotViews.back().name, sizeof(_plotViews.back().name), "Plot");
}

void
appMain::drawPlots()
{
    if (_phase != Phase::Active) { return; }
    int itemToRemoveIdx = -1;

    for (int plotIdx = 0; plotIdx < (int)_plotViews.size(); ++plotIdx) {
        auto& pv = _plotViews[plotIdx];

        char tmpStr[256];
        snprintf(tmpStr, sizeof(tmpStr), "%s###%d", pv.name, pv.uniqueId);
        bool isOpen = true;
        if (ImGui::Begin(tmpStr, &isOpen,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs)) {
            drawPlot(pv);
        }
        if (!isOpen) itemToRemoveIdx = plotIdx;
        ImGui::End();
    }

    // Remove profile if needed
    if (itemToRemoveIdx >= 0) {
        _plotViews.erase(_plotViews.begin() + itemToRemoveIdx);
        dirty();
    }
}

void
appMain::preparePlotData(PlotView& pv)
{
    pv.isDataDirty = false;
    pv.xs.clear();
    pv.ys.clear();

    // Collect the logs
    std::string      errorMessage;
    std::vector<int> valuePositions;
    if (!_logSession.query(
            pv.rules,
            [this, &pv](int /*ruleIdx*/, const sslogread::LogStruct& log) {
                // Filter on the format string and arg qty (sanity)
                if ((int)log.formatIdx != pv.formatIdx || (int)log.args.size() < pv.argIdx) { return true; }

                pv.xs.push_back(1e-9 * (double)log.timestampUtcNs);

                const sslogread::Arg& arg = log.args[pv.argIdx];
                switch (arg.pType) {
                    case sslogread::ArgType::S32:
                        pv.ys.push_back((double)arg.vS32);
                        break;
                    case sslogread::ArgType::U32:
                        pv.ys.push_back((double)arg.vU32);
                        break;
                    case sslogread::ArgType::S64:
                        pv.ys.push_back((double)arg.vS64);
                        break;
                    case sslogread::ArgType::U64:
                        pv.ys.push_back((double)arg.vU64);
                        break;
                    case sslogread::ArgType::Float:
                        pv.ys.push_back((double)arg.vFloat);
                        break;
                    case sslogread::ArgType::Double:
                        pv.ys.push_back(arg.vDouble);
                        break;
                    case sslogread::ArgType::StringIdx:
                        pv.ys.push_back((double)arg.vStringIdx);  // @FIXME This is temporary, a new kind of plot must be done for Strings
                };

                return true;
            },
            errorMessage)) {
        fprintf(stderr, "Error: %s\n", errorMessage.c_str());
    }

    asserted(pv.xs.size() == pv.ys.size(), "By design");
}

template<typename T>
int
BinarySearch(const T* arr, int l, int r, T x)
{
    if (r >= l) {
        int mid = l + (r - l) / 2;
        if (arr[mid] == x) { return mid; }
        if (arr[mid] > x) { return BinarySearch(arr, l, mid - 1, x); }
        return BinarySearch(arr, mid + 1, r, x);
    }
    return l;
}

void
appMain::drawPlot(PlotView& pv)
{
    if (pv.isDataDirty) { preparePlotData(pv); }

    if (ImPlot::BeginPlot("My Plot", ImVec2(-1, -1))) {
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
        ImPlot::PlotLine("My Line Plot", pv.xs.data(), pv.ys.data(), pv.xs.size());

        // Get the closest point
        double mouseX = ImPlot::GetPlotMousePos().x;
        int    idx    = BinarySearch(pv.xs.data(), 0, (int)pv.xs.size() - 1, mouseX);
        if (idx + 1 < (int)pv.xs.size() && bsAbs(mouseX - pv.xs[idx + 0]) > bsAbs(mouseX - pv.xs[idx + 1])) idx = idx + 1;

        // Check the proximity
        // @TODO Need plot resolution...
        if (idx >= 0) {
            printf("FOUND\n");
            ImPlot::Annotation(pv.xs[idx], pv.ys[idx], ImVec4(1., 1., 1., 1.), ImVec2(10, 10), true, "Point %d", idx);
        }

        ImPlot::EndPlot();
    }
}
