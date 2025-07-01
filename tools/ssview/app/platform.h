#pragma once

// System
#include <atomic>

// Internal
#include "bs.h"
#include "bsTime.h"
#include "keycode.h"
#include "os.h"

#define VW_REDRAW_PER_NTF    5
#define VW_REDRAW_PER_BOUNCE 2

class vwMain;

class vwPlatform : public os::Handler
{
   public:
    // Constructor & destructor
    vwPlatform(const bsString& filename);
    virtual ~vwPlatform(void);
    void run(void);

    // Application interface
    void   quit(void) { _doExit.store(1); }
    bsUs_t getLastUpdateDuration(void) const { return _lastUpdateDurationUs; }
    bsUs_t getLastRenderingDuration(void) const { return _lastRenderingDurationUs; }
    bsUs_t getLastMouseMoveDurationUs(void) const { return bsGetClockUs() - _lastMouseMoveTimeUs; }
    int    getDisplayWidth(void) const { return _displayWidth; }
    int    getDisplayHeight(void) const { return _displayHeight; }
    bool   captureScreen(int* width, int* height, uint8_t** buffer);

    // Event handling
    bool redraw();
    void notifyDrawDirty(void) { _dirtyRedrawCount.store(VW_REDRAW_PER_NTF); }
    void notifyWindowSize(int windowWidth, int windowHeight)
    {
        _displayWidth  = windowWidth;
        _displayHeight = windowHeight;
        notifyDrawDirty();
    }
    bool isVisible(void) const { return _isVisible.load(); }

    // Events
    void notifyMapped(void) { notifyDrawDirty(); }
    void notifyUnmapped(void) { notifyDrawDirty(); }
    void notifyExposed(void) { notifyDrawDirty(); }
    void notifyFocusOut(void) { notifyDrawDirty(); }
    void notifyEnter(os::KeyModState kms);
    void notifyLeave(os::KeyModState kms);

    void eventChar(uint16_t codepoint);
    void eventKeyPressedOrReleased(os::Keycode keycode, os::KeyModState kms, bool pressState);
    void eventKeyPressed(os::Keycode keycode, os::KeyModState kms);
    void eventKeyReleased(os::Keycode keycode, os::KeyModState kms);
    void eventButtonPressed(int buttonId, int x, int y, os::KeyModState kms);
    void eventButtonReleased(int buttonId, int x, int y, os::KeyModState kms);
    void eventMouseMotion(int x, int y);
    void eventWheelScrolled(int x, int y, int steps, os::KeyModState kms);

   private:
    vwPlatform(const vwPlatform& other);  // To please static analyzers
    vwPlatform& operator=(vwPlatform other);
    void        configureStyle(void);

    // Platform state
    std::atomic<int>      _doExit;
    std::atomic<int>      _isVisible;
    std::atomic<uint64_t> _dirtyRedrawCount;
    vwMain*               _main                = 0;
    bsUs_t                _lastMouseMoveTimeUs = 0;

    // ImGui
    int    _displayWidth            = -1;
    int    _displayHeight           = -1;
    float  _dpiScale                = 1.;
    bsUs_t _lastUpdateDurationUs    = 1;  // Update only
    bsUs_t _lastRenderingDurationUs = 1;  // Update and rendering
    bsUs_t _lastRenderingTimeUs     = 0;
};
