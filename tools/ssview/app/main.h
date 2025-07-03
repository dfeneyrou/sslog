#pragma once

// System
#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

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
class appPlatform;

// Helpers
#define DRAWLIST ImGui::GetWindowDrawList()

class appMain
{
   public:
    // Constructor & destructor
    appMain(appPlatform* platform, const bsString& filename);
    ~appMain();
    void notifyStart();

    // Application
    void draw();
    int  getFontSize(void) { return _settingsView.fontSize; }

   private:
    // Private context
    enum class Phase : int { InitFont, WaitForFilename, InitiateFileLoading, LoadingFile, Active };
    std::atomic<Phase> _phase{Phase::InitFont};
    std::atomic<int>   _phaseCompletionFlag{0};
    std::thread        _workerThread;
    int64_t            _mouseTimeNs = 0.;

    appMain(const appMain& other);  // To please static analyzers
    appMain& operator=(appMain other);

    void drawMainMenuBar();
    void drawHelp();
    void drawAbout();
    void drawSettings();

    int    getDisplayWidth();
    int    getDisplayHeight();
    void   dirty();  // Force several frame of redrawing
    int    getId(void) { return _generatorUniqueId++; }
    bsUs_t getLastMouseMoveDurationUs(void) const { return _lastMouseMoveDurationUs; }

    void openHelpTooltip(int uniqueId, const char* tooltipId);
    void displayHelpText(const char* helpStr);
    void displayHelpTooltip(int uniqueId, const char* tooltipId, const char* helpStr);
    bool loadSession();

    struct TextElem {
        uint64_t             timestampUtcNs;
        sslog::Level         level;
        uint32_t             threadIdx;
        uint32_t             categoryIdx;
        std::string          message;
        std::vector<int>     valuePositions;
        std::vector<uint8_t> buffer;
    };
    struct TextView {
        uint32_t                     uniqueId = 0;
        char                         name[64];
        float                        maxCategoryLength      = 0;
        float                        maxThreadLength        = 0;
        float                        lengthFontSizeRef      = 1.;
        bool                         isDataDirty            = true;
        bool                         isNew                  = true;
        int64_t                      targetScrollDateNs     = -1;
        int64_t                      rangeSelStartNs        = -1;
        float                        rangeSelStartY         = 0.;
        bool                         doDisplayLevel         = true;
        bool                         doDisplayThread        = true;
        bool                         doDisplayCategory      = true;
        bool                         doDisplayBufferContent = true;
        std::vector<sslogread::Rule> rules;
        std::vector<TextElem>        cachedElems;
    };
    void                         addTextView(uint32_t id);
    void                         drawTexts();
    void                         drawText(TextView& tv);
    void                         drawTextFilterConfig(TextView& tv);
    void                         prepareTextData(TextView& tv);
    std::vector<sslogread::Rule> _rulesUnderWork;
    std::vector<TextView>        _textViews;

    appPlatform*       _platform                = 0;
    uint32_t           _generatorUniqueId       = 1;
    bsUs_t             _lastMouseMoveDurationUs = 0;
    std::vector<ImU32> _colorPalette;

    // Log session fields
    bsString              _filename;
    sslogread::LogSession _logSession;
    os::Date              _originDate;
    int64_t               _originUtcNs    = 0;
    int64_t               _dayOriginUtcNs = 0;
    int64_t               _tzOffsetNs     = 0;

    // About
    bool _showAbout = false;

    // Settings
    struct SettingsView {
        uint32_t uniqueId = 0;
        bool     isOpen   = false;
        // Settings
        TimeFormat timeFormat = TimeFormat::HhMmSsNanosecond;
        bool       useUtc     = false;
        int        fontSize   = FontSizeDefault;
        int        colorSeed  = 0;
    };
    SettingsView _settingsView;

    // File dialogs
    appFileDialog* _fileDialogLoadLogs = nullptr;
    bsString       _lastPath;
    std::string    _fileLoadErrorMsg;
};
