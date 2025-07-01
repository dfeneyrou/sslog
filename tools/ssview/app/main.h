#pragma once

// System
#include <atomic>
#include <functional>
#include <mutex>

// External
#include "imgui.h"
#include "sslogread/sslogread.h"
#include "sslogread/utils.h"

// Internal
#include "appCommon.h"
#include "bs.h"
#include "bsString.h"
#include "bsTime.h"
#include "fileDialog.h"

// Forward declarations
class vwPlatform;

// Helpers
#define DRAWLIST ImGui::GetWindowDrawList()

class vwMain  // @RENAME
{
   public:
    // Constructor & destructor
    vwMain(vwPlatform* platform, const bsString& filename);
    ~vwMain();
    void notifyStart();

    // Application
    void draw();
    int  getFontSize(void) { return _settingsView.fontSize; }

   private:
    enum class Phase : int { InitFont, WaitForFilename, InitiateFileLoading, LoadingFile, Active };
    std::atomic<Phase> _phase{Phase::InitFont};
    std::atomic<int>   _phaseCompletionFlag{0};
    std::thread        _workerThread;
    int64_t            _mouseTimeNs = 0.;

    vwMain(const vwMain& other);  // To please static analyzers
    vwMain& operator=(vwMain other);

    void drawMainMenuBar();
    void drawHelp();
    void drawAbout();
    void drawSettings();

    // Other shared methods
    int  getDisplayWidth();
    int  getDisplayHeight();
    void dirty();  // Force several frame of redrawing
    int  getId(void) { return _generatorUniqueId++; }

    void openHelpTooltip(int uniqueId, const char* tooltipId);
    void displayHelpText(const char* helpStr);
    void displayHelpTooltip(int uniqueId, const char* tooltipId, const char* helpStr);

    struct LogElem {
        uint64_t    timestampUtcNs;
        std::string category;
        std::string message;
    };
    struct LogView {
        uint32_t       uniqueId = 0;
        uint64_t       originUtcNs;
        int            maxCategoryLength = 0;
        bool           isDataDirty       = true;
        bool           isNew             = true;
        int64_t        rangeSelStartNs   = -1;
        float          rangeSelStartY    = 0.;
        bsVec<LogElem> cachedLogs;
    };
    void                 addLogView(uint32_t id);
    void                 drawLogs();
    void                 drawLog(LogView& lv);
    void                 prepareLogData(LogView& lv);
    std::vector<LogView> _logViews;

    vwPlatform*           _platform = 0;
    bsString              _filename;
    sslogread::LogSession _logSession;
    uint32_t              _generatorUniqueId = 1;

    // About
    bool _showAbout = false;

    // Settings
    struct SettingsView {
        uint32_t uniqueId = 0;
        bool     isOpen   = false;
        // Settings
        int timeFormat = TIME_FORMAT_HHMMSS;
        int fontSize   = FontSizeDefault;
    };
    SettingsView _settingsView;

    // File dialogs
    vwFileDialog* _fileDialogLoadLogs = nullptr;
    bsString      _lastPath;
    std::string   _fileLoadErrorMsg;

    // Log window
};
