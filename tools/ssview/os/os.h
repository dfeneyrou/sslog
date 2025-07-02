#pragma once

#include <cstdio>
#include <vector>

#include "bs.h"
#include "bsString.h"
#include "keycode.h"

// Main function, to be implemented in the main app
int
osBootstrapImpl(int argc, char* argv[]);

namespace os
{

// Event handler interface
class Handler
{
   public:
    Handler() = default;
    virtual ~Handler() {}
    // Events
    virtual void notifyWindowSize(int windowWidth, int windowHeight)              = 0;
    virtual void notifyMapped()                                                   = 0;
    virtual void notifyUnmapped()                                                 = 0;
    virtual void notifyExposed()                                                  = 0;
    virtual void notifyFocusOut()                                                 = 0;
    virtual void notifyEnter(KeyModState kms)                                     = 0;
    virtual void notifyLeave(KeyModState kms)                                     = 0;
    virtual void eventChar(uint16_t codepoint)                                    = 0;
    virtual void eventKeyPressed(Keycode keycode, KeyModState kms)                = 0;
    virtual void eventKeyReleased(Keycode keycode, KeyModState kms)               = 0;
    virtual void eventButtonPressed(int buttonId, int x, int y,
                                    KeyModState kms)                              = 0;  // 0=left, 1=middle, 2=right
    virtual void eventButtonReleased(int buttonId, int x, int y, KeyModState kms) = 0;
    virtual void eventMouseMotion(int x, int y)                                   = 0;
    virtual void eventWheelScrolled(int x, int y, int steps, KeyModState kms)     = 0;
    // Others
    virtual bool isVisible() const = 0;
    virtual void quit()            = 0;
};

// Date structure
struct Date {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0,
        second = 0;  // As displayed (no offset)
    bool isOlderThan(const Date& o) const
    {
        if (year < o.year) { return true; }
        if (year > o.year) { return false; }
        if (month < o.month) { return true; }
        if (month > o.month) { return false; }
        if (day < o.day) { return true; }
        if (day > o.day) { return false; }
        if (hour < o.hour) { return true; }
        if (hour > o.hour) { return false; }
        if (minute < o.minute) { return true; }
        if (minute > o.minute) { return false; }
        if (second < o.second) {
            return true;
        } else
            return false;
    }
    bool isEmpty() const { return (year == 0); }
};

// OS abstraction layer, to be used by applications
// ================================================

void
createWindow(const char* windowTitle, const char* configName, float ratioLeft, float ratioTop, float ratioRight, float ratioBottom,
             bool overrideWindowManager = false);

void
destroyWindow();

void
setWindowTitle(const bsString& title);

void
getWindowSize(int& width, int& height, int& dpiWidth, int& dpiHeight);

void
processInputs(Handler* osHandler);

void
hideWindow();

void
showWindow();

void
swapBuffer();

void
setMouseVisible(bool state);

bool
isMouseVisible();

// Clipboard
enum ClipboardType { NONE, UTF8, APP_INTERNAL };

void
pushToClipboard(ClipboardType pushType, const bsStringUtf16& data);

bsStringUtf16
reqFromClipboard(ClipboardType reqType);

// File system
FILE*
fileOpen(const bsString& path, const char* mode);

bsString
getProgramDataPath();  // UTF-8

enum DirStatusCode { OK, FAILURE, DOES_NOT_EXIST, NOT_A_DIRECTORY, PERMISSION_DENIED, ALREADY_EXISTS };

struct DirEntry {
    bsString name;
    bool     isDir;  // If false, it is a file
};

Date
getDate();

DirStatusCode
getDirContent(const bsString& path, std::vector<DirEntry>& entries);

DirStatusCode
makeDir(const bsString& path);

bool
fileExists(const bsString& path);

bool
directoryExists(const bsString& path);

size_t
getSize(const bsString& path);

Date
getCreationDate(const bsString& path);

bool
loadFileContent(const bsString& path, std::vector<uint8_t>& buffer, int maxSize = -1);

DirStatusCode
removeFile(const bsString& path);

DirStatusCode
removeDir(const bsString& path, bool onlyIfEmpty = true);

bool
copyFile(const bsString& srcPath, const bsString& dstPath);

uint32_t
getDriveBitmap();

bsString
getBasename(const bsString& path);

bsString
getDirname(const bsString& path);

bsString
getCurrentPath();

void
setIcon(int width, int height, const uint8_t* pixels);  // Size of pixels array is 4*width*height (RGBA)

// Some portability fixes
// ======================
#ifdef _WIN32

// Windows case
#define PL_DIR_SEP      "\\"
#define PL_DIR_SEP_CHAR '\\'

#if !defined(strcasecmp)
#define strcasecmp _stricmp
#endif

// strcasestr does not exists on windows
const char*
strcasestr(const char* s, const char* sToFind);

// ftell and fseek are limited to 32 bits offset, so the 64 bits version is used here
#define bsOsFseek _fseeki64
#define bsOsFtell _ftelli64

#else  // ifdef _WIN32

// Linux case
#define PL_DIR_SEP      "/"
#define PL_DIR_SEP_CHAR '/'

// ftell and fseek support big files directly
#define bsOsFseek       fseek
#define bsOsFtell       ftell

#endif  // ifdef _WIN32

};  // namespace os
