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

#include <cstring>

#include "sslogread/sslogread.h"
#include "sslogread/utils.h"

namespace sslogread
{

void
LogSession::setStringBuffer(std::vector<uint8_t>&& newStringBuffer)
{
    // Store internally the new string buffer
    _stringsBuffer = std::move(newStringBuffer);

    // Analyze the start of each string
    uint32_t readOffset  = 0;
    uint32_t startOffset = 0;
    _stringsStartOffsets.clear();
    while (readOffset < _stringsBuffer.size()) {
        while (readOffset < _stringsBuffer.size() && _stringsBuffer[readOffset] != 0) ++readOffset;
        _stringsStartOffsets.push_back(startOffset);
        startOffset = ++readOffset;  // Skip the zero termination
    }

    _stringsFlags.clear();
    _stringsFlags.resize(_stringsStartOffsets.size(), 0);
    _isMetaInfoPresent = false;

    // Analyze the arguments of the detected parameters
    _stringsArgs.clear();
    for (uint32_t sIdx = 0; sIdx < _stringsStartOffsets.size(); ++sIdx) {
        const char* sStart = getIndexedString(sIdx);
        const char* s      = sStart;
        _stringsArgs.push_back({});

        // Loop on string's chars
        while (*s != 0) {
            // Find the start of the next parameter
            while (*s != 0 && *s != '%' && *s != '{') ++s;
            if (*s == 0) { break; }
            if ((*s == '%' || *s == '{') && *(s + 1) == *s) {
                s += 2;
                continue;
            }  // Escapes

            // Find the name = prefix, space separated
            const char* name = (s - 1 > sStart) ? (s - 1) : sStart;
            while (name > sStart && (*name == '_' || *name == '=' || *name == ':' || *name == '-')) --name;
            const char* nameEnd = name + 1;  // Excluded
            while (name > sStart && *name != ' ') --name;
            if (*name == ' ') ++name;

            // Find the unit
            const char* unit = s + 1;
            while (*unit != 0 &&
                   ((*unit >= '0' && *unit <= '9') || *unit == '.' || *unit == '-' || *unit == '+' || *unit == '*' || *unit == 'l'))
                ++unit;
            if (*unit != 0) ++unit;  // Pass the terminating '}' or the final type letter in printf format (ex: %08d)
            while (*unit != 0 && (*unit == '_' || *unit == '=' || *unit == ':' || *unit == '-')) ++unit;
            const char* unitEnd = unit;
            while (*unitEnd != 0 && *unitEnd != ' ' && *unitEnd != '.' && *unitEnd != ',' && *unitEnd != ';' && *unitEnd != '!' &&
                   *unitEnd != '?')
                ++unitEnd;

            _stringsArgs.back().push_back({std::string(name, nameEnd), std::string(unit, unitEnd)});
            s = unitEnd;
        }
    }
}

std::vector<uint32_t>
LogSession::getStringIndexes(const std::string& pattern, uint8_t stringFlagsMask) const
{
    if (!_isMetaInfoPresent && stringFlagsMask != 0) {
        std::string errorMessage;
        extractMetaInfos(errorMessage);
    }

    // Analyze the pattern
    std::vector<PatternChunk> patternChunks = getPatternChunks(pattern);

    // Search in all strings
    std::vector<uint32_t> indexes;
    for (uint32_t stringIdx = 0; stringIdx < _stringsStartOffsets.size(); ++stringIdx) {
        if ((_stringsFlags[stringIdx] & stringFlagsMask) != 0 && isStringMatching(patternChunks, getIndexedString(stringIdx))) {
            indexes.push_back(stringIdx);
        }
    }

    return indexes;
}

std::vector<std::string>
LogSession::getArgNameStrings() const
{
    if (!_isMetaInfoPresent) {
        std::string errorMessage;
        extractMetaInfos(errorMessage);
    }

    sslog::priv::FlatHashTable<uint32_t> lkupArgName;  // To keep strings unique
    uint32_t                             dummy = 0;
    std::vector<std::string>             outputList;

    for (uint32_t stringIdx = 0; stringIdx < _stringsFlags.size(); ++stringIdx) {
        if ((_stringsFlags[stringIdx] & sslogread::FlagFormat) == 0) { continue; }

        // Loop on the arguments infos for this format string
        for (const sslogread::ArgNameAndUnit& argInfo : _stringsArgs[stringIdx]) {
            if (argInfo.name.empty()) { continue; }

            sslog::priv::hashStr_t strHash = sslog::priv::hashString(argInfo.name.c_str());
            if (!lkupArgName.find(strHash, dummy)) {
                lkupArgName.insert(strHash, 1);
                outputList.push_back(argInfo.name);
            }
        }
    }
    return outputList;
}

std::vector<std::string>
LogSession::getArgUnitStrings() const
{
    if (!_isMetaInfoPresent) {
        std::string errorMessage;
        extractMetaInfos(errorMessage);
    }

    sslog::priv::FlatHashTable<uint32_t> lkupArgUnit;  // To keep strings unique
    uint32_t                             dummy = 0;
    std::vector<std::string>             outputList;

    for (uint32_t stringIdx = 0; stringIdx < _stringsFlags.size(); ++stringIdx) {
        if ((_stringsFlags[stringIdx] & sslogread::FlagFormat) == 0) { continue; }

        // Loop on the arguments infos for this format string
        for (const sslogread::ArgNameAndUnit& argInfo : _stringsArgs[stringIdx]) {
            if (argInfo.unit.empty()) { continue; }

            sslog::priv::hashStr_t strHash = sslog::priv::hashString(argInfo.unit.c_str());
            if (!lkupArgUnit.find(strHash, dummy)) {
                lkupArgUnit.insert(strHash, 1);
                outputList.push_back(argInfo.unit);
            }
        }
    }
    return outputList;
}

uint64_t
LogSession::getLogQty() const
{
    if (!_isMetaInfoPresent) {
        std::string errorMessage;
        extractMetaInfos(errorMessage);
    }
    return _statLogs;
}

uint64_t
LogSession::getArgQty() const
{
    if (!_isMetaInfoPresent) {
        std::string errorMessage;
        extractMetaInfos(errorMessage);
    }
    return _statArgs;
}

uint64_t
LogSession::getLogByteQty() const
{
    if (!_isMetaInfoPresent) {
        std::string errorMessage;
        extractMetaInfos(errorMessage);
    }
    return _statLogBytes;
}

uint64_t
LogSession::getLogDurationNs() const
{
    if (!_isMetaInfoPresent) {
        std::string errorMessage;
        extractMetaInfos(errorMessage);
    }
    return _statDurationNs;
}

bool
LogSession::extractMetaInfos(std::string& errorMessage) const
{
    if (_isMetaInfoPresent) { return true; }
    std::vector<uint8_t>& flags = _stringsFlags;

    _statLogs           = 0;
    _statArgs           = 0;
    _statLogBytes       = _statBaseBytes;
    _doSumFileStatSizes = true;
    uint64_t minTimeNs  = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t maxTimeNs  = 0;

    bool queryStatus = query(
        {},
        [this, &flags, &minTimeNs, &maxTimeNs](int /*ruleIdx*/, const sslogread::LogStruct& log) {
            if (log.categoryIdx < flags.size()) { flags[log.categoryIdx] |= sslogread::FlagCategory; }
            if (log.threadIdx < flags.size()) { flags[log.threadIdx] |= sslogread::FlagThread; }
            if (log.formatIdx < flags.size()) { flags[log.formatIdx] |= sslogread::FlagFormat; }

            if (log.timestampUtcNs < minTimeNs) { minTimeNs = log.timestampUtcNs; }
            if (log.timestampUtcNs > maxTimeNs) { maxTimeNs = log.timestampUtcNs; }
            _statLogs += 1;
            _statArgs += log.args.size();

            for (const sslogread::Arg& arg : log.args) {
                if (arg.pType == sslogread::ArgType::StringIdx && arg.vStringIdx < flags.size()) {
                    flags[arg.vStringIdx] |= sslogread::FlagArgValue;
                }
            }
        },
        errorMessage);

    _statDurationNs     = (maxTimeNs >= minTimeNs) ? maxTimeNs - minTimeNs : 0;
    _doSumFileStatSizes = false;
    if (!queryStatus) { return false; }
    _isMetaInfoPresent = true;

    return true;
}

}  // namespace sslogread
