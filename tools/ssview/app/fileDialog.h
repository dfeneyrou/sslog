
#pragma once

// Internal
#include <vector>

#include "bsString.h"
#include "os.h"

class appFileDialog
{
   public:
    enum Mode { SELECT_DIR, OPEN_FILE, SAVE_FILE };
    appFileDialog(const bsString& title, Mode mode, const std::vector<bsString>& typeFilters);

    void open(const bsString& initialPath, int maxSelectionQty = 1);
    void close(void) { _shallClose = true; }
    bool draw(void);

    bool hasSelection(void) const { return _hasSelection; }
    void clearSelection(void)
    {
        _hasSelection = false;
        _selection.clear();
    }
    const std::vector<bsString>& getSelection(void)
    {
        asserted(hasSelection());
        return _selection;
    }

   private:
    struct Entry {
        bsString name;
        os::Date date;
        int64_t  size;
        bool     isSelected;
    };
    static constexpr int MAX_WRITE_SELECTION_SIZE = 256;

    bsString              _title;
    bsString              _path;
    bsString              _displayedSelection;
    char                  _modifiableSelection[MAX_WRITE_SELECTION_SIZE] = {0};
    Mode                  _mode;
    std::vector<bsString> _typeFilters;
    std::vector<Entry>    _dirEntries;
    std::vector<Entry>    _fileEntries;
    std::vector<bsString> _selection;
    int                   _selectedFilterIdx = 0;
    uint32_t              _driveBitMap       = 0;
    bool                  _isEntriesDirty    = true;
    bool                  _isSelDisplayDirty = true;
    bool                  _doShowHidden      = false;
    bool                  _isOpen            = false;
    bool                  _shallOpen         = false;
    bool                  _shallClose        = false;
    bool                  _hasSelection      = false;
    int                   _maxSelectionQty   = 1;
};
