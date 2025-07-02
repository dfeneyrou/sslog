// The MIT License (MIT)
//
// Copyright(c) 2025, Damien Feneyrou <dfeneyrou@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#define SSLOG_NO_BOOTSTRAP
#include "sslog.h"

namespace sslogread
{

// Definition of the filtering rules.
// Each field operates on a particular information of the log, and all the constraints must be
// fulfilled (logic AND).
// The "no" prefix means a filter-out (exclusion) in case of match.
// Strings are handled as a pattern matching the full field. Wildcards are accepted.
struct Rule {
    sslog::Level             levelMin = sslog::Level::trace;
    sslog::Level             levelMax = sslog::Level::off;
    std::string              category;
    std::string              thread;
    std::string              format;
    std::vector<std::string> arguments;
    uint32_t                 bufferSizeMin = 0;
    uint32_t                 bufferSizeMax = 0xFFFFFFFF;

    std::string noCategory;
    std::string noThread;
    std::string noFormat;
};

// Structure representing the typed value of an argument
enum class ArgType { S32 = 0, U32, S64, U64, Float, Double, StringIdx };
struct Arg {
    enum ArgType pType;
    union {
        int32_t  vS32;
        uint32_t vU32;
        int64_t  vS64;
        uint64_t vU64;
        float    vFloat;
        double   vDouble;
        uint32_t vStringIdx;
    };
};

// Structure representing the name and unit of an argument of a format string
struct ArgNameAndUnit {
    std::string name;
    std::string unit;
};

// Structure exposing all information of a log
// The name and unit of arguments can be retrieved via the StringDb and the formatIdx
struct LogStruct {
    sslog::Level         level;
    uint64_t             timestampUtcNs;
    uint32_t             categoryIdx;
    uint32_t             threadIdx;
    uint32_t             formatIdx;
    std::vector<Arg>     args;
    std::vector<uint8_t> buffer;
};

// Flags associated to the kind of usage of a string in logs. May be multiple usage.
enum StringFlags { FlagNone = 0x0, FlagCategory = 0x1, FlagThread = 0x2, FlagFormat = 0x4, FlagArgValue = 0x8 };

class LogSession
{
   public:
    LogSession()  = default;
    ~LogSession() = default;
    bool init(const std::filesystem::path& logDirPath, std::string& errorMessage);

    // Main service
    bool query(const std::vector<Rule>& rules, const std::function<bool(int, const LogStruct&)>& callback, std::string& errorMessage) const;

    // Strings
    size_t      getIndexedStringQty() const { return _stringsStartOffsets.size(); }
    const char* getIndexedString(uint32_t stringIndex) const
    {
        return (stringIndex < _stringsStartOffsets.size()) ? (const char*)&_stringsBuffer[_stringsStartOffsets[stringIndex]] :
                                                             "<<<Corrupted string>>>";
    }
    const std::vector<ArgNameAndUnit>& getIndexedStringArgNameAndUnit(uint32_t stringIndex) const
    {
        static std::vector<ArgNameAndUnit> emptyArray;
        return (stringIndex < _stringsArgs.size()) ? _stringsArgs[stringIndex] : emptyArray;
    }
    uint8_t getIndexedStringFlags(uint32_t stringIndex) const
    {
        if (!_isMetaInfoPresent) {
            std::string errorMessage;
            extractMetaInfos(errorMessage);
        }
        return (stringIndex < _stringsFlags.size()) ? _stringsFlags[stringIndex] : 0;
    }
    std::vector<uint32_t>    getStringIndexes(const std::string& pattern, uint8_t stringFlagsMask = 0xFF) const;
    std::vector<std::string> getArgNameStrings() const;
    std::vector<std::string> getArgUnitStrings() const;

    // Misc
    uint64_t getUtcSystemClockOriginNs() const { return _utcSystemClockStartNs; }
    double   getClockResolutionNs() const { return _tickToNs; }
    uint64_t getLogQty() const;
    uint64_t getArgQty() const;
    uint64_t getLogByteQty() const;
    uint64_t getLogDurationNs() const;

    static const char* getLevelName(sslog::Level level) { return sslog::priv::getLevelName(level); }

   private:
    static constexpr int BaseHeaderSize = 32;
    static constexpr int DataHeaderSize = 32;

    // Structures and internal classes
    struct DataInfos {
        uint32_t             formatVersion          = 0;
        uint64_t             sessionId              = 0;
        uint64_t             utcSystemClockOriginNs = 0;
        uint64_t             steadyClockOriginTick  = 0;
        std::vector<uint8_t> logBuffer;
    };

    // Helper to mix different streams
    class LogStream
    {
       public:
        LogStream(const LogSession* session, const DataInfos& dataInfos, sslog::priv::TimeConverter& timeConverter);
        bool empty() const { return _isEndReached; }
        void readNext();

        // Output
        LogStruct output;

       private:
        // Input
        const LogSession*           _session                = nullptr;
        const uint8_t*              _buffer                 = nullptr;
        uint32_t                    _bufferSize             = 0;
        uint64_t                    _utcSystemClockOriginNs = 0;
        uint64_t                    _steadyClockOriginTick  = 0;
        sslog::priv::TimeConverter& _timeConverter;

        // Internal
        bool     _isEndReached      = false;
        uint32_t _offset            = 0;
        uint32_t _lastCategoryIdx   = 0;
        uint32_t _lastFormatIdx     = 0;
        uint32_t _lastThreadId      = 0;
        uint64_t _lastTimestampTick = 0;
    };

    // Helper to filter logs
    class Filter
    {
       public:
        Filter(const LogSession* session, const Rule& rule);
        bool apply(const LogStream* ls) const;

       private:
        struct ArgMultiValues {
            std::string name;
            bool        checkEqual   = false;
            bool        checkGreater = false;
            bool        checkLesser  = false;
            bool        hasNumeric   = false;
            int64_t     vS64         = 0;
            uint64_t    vU64         = 0;
            double      vDouble      = 0.;
            std::string vString;
        };
        bool parseArgument(const std::string& a, ArgMultiValues& mv);

        // Fields
        bool                                              _filterLevel[SSLOG_LEVEL_QTY] = {false};
        sslog::priv::FlatHashTable<bool>                  _filterCategory;
        sslog::priv::FlatHashTable<bool>                  _filterCategoryExcl;
        sslog::priv::FlatHashTable<bool>                  _filterThread;
        sslog::priv::FlatHashTable<bool>                  _filterThreadExcl;
        sslog::priv::FlatHashTable<bool>                  _filterFormat;
        sslog::priv::FlatHashTable<bool>                  _filterFormatExcl;
        sslog::priv::FlatHashTable<std::vector<uint64_t>> _filterArgPosition;
        std::vector<ArgMultiValues>                       _filterArgValue;

        const LogSession* _session = nullptr;
        Rule              _rule;
    };

    // Internal methods
    bool readBaseInfos(const std::filesystem::path& logDirPath, std::string& errorMessage);
    bool readDataInfos(const std::filesystem::path& dataFilePath, DataInfos& infos, std::string& errorMessage) const;
    void setStringBuffer(std::vector<uint8_t>&& newStringBuffer);
    bool extractMetaInfos(std::string& errorMessage) const;
    bool loadUncompressedFile(const std::filesystem::path& filePath, std::string& errorMessage, std::vector<uint8_t>& buffer) const;
    bool loadCompressedFile(const std::filesystem::path& filePath, std::string& errorMessage, std::vector<uint8_t>& buffer) const;

    // Session meta information
    uint32_t     _formatVersion         = 0;
    uint64_t     _sessionId             = 0;
    uint64_t     _utcSystemClockStartNs = 0;
    double       _tickToNs              = 1.;
    mutable bool _doSumFileStatSizes    = false;

    // Stats
    mutable uint64_t _statLogs       = 0;
    mutable uint64_t _statArgs       = 0;
    mutable uint64_t _statLogBytes   = 0;
    mutable uint64_t _statBaseBytes  = 0;
    mutable uint64_t _statDurationNs = 0;

    // List of data files
    std::vector<std::string> _dataFileList;

    // String management
    // StringDb _strings;
    std::vector<uint8_t>                     _stringsBuffer;        // Buffer containing all the concatenated zero-terminated strings
    std::vector<uint32_t>                    _stringsStartOffsets;  // Array of indexes of the string start offsets
    std::vector<std::vector<ArgNameAndUnit>> _stringsArgs;          // Array of per-string list of arguments
    mutable std::vector<uint8_t>             _stringsFlags;         // Array of flags per string
    mutable bool                             _isMetaInfoPresent = false;
};

}  // namespace sslogread
