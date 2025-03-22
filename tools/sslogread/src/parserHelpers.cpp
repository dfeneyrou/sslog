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

#include <climits>
#include <string>
#include <vector>

#include "sslogread/sslogread.h"
#include "sslogread/utils.h"

namespace sslogread
{

LogSession::LogStream::LogStream(const LogSession* session, const DataInfos& dataInfos, sslog::priv::TimeConverter& timeConverter)
    : _session(session),
      _buffer(dataInfos.logBuffer.data()),
      _bufferSize((uint32_t)dataInfos.logBuffer.size()),
      _timeConverter(timeConverter)
{
    _utcSystemClockOriginNs = dataInfos.utcSystemClockOriginNs;
    _steadyClockOriginTick  = dataInfos.steadyClockOriginTick;
    readNext();
}

void
LogSession::LogStream::readNext()
{
    // End of stream?
    if (_offset + 1 >= _bufferSize) {
        _isEndReached = true;
        return;
    }

    // Parse the fixed header
    const uint8_t* bl       = &_buffer[_offset];
    uint16_t       flagType = (*bl++);
    flagType |= (*bl++) << 8;
    output.level                = sslog::Level((flagType >> SSLOG_LEVEL_SHIFT) & SSLOG_LEVEL_MASK);
    int      formatIdxByteQty   = (flagType >> SSLOG_FORMAT_SHIFT) & SSLOG_FORMAT_MASK;  // Direct mapping
    int      categoryIdxByteQty = (flagType >> SSLOG_CATEGORY_SHIFT) & SSLOG_CATEGORY_MASK;
    int      threadIdxByteQty   = (flagType >> SSLOG_THREADIDX_SHIFT) & SSLOG_THREADIDX_MASK;
    uint32_t tsCode             = (flagType >> SSLOG_TS_SHIFT) & SSLOG_TS_MASK;
    int      timestampByteQty   = 8;
    if (tsCode == (SSLOG_TS_2_BYTES >> SSLOG_TS_SHIFT)) {
        timestampByteQty = 2;
    } else if (tsCode == (SSLOG_TS_3_BYTES >> SSLOG_TS_SHIFT)) {
        timestampByteQty = 3;
    } else if (tsCode == (SSLOG_TS_4_BYTES >> SSLOG_TS_SHIFT)) {
        timestampByteQty = 4;
    }
    int headerSize = 2 + formatIdxByteQty + categoryIdxByteQty + threadIdxByteQty + timestampByteQty;

    output.formatIdx = _lastFormatIdx & (0xFFFFFFFF << (8 * formatIdxByteQty));  // Keep only the previous highest bits
    for (int i = 0; i < formatIdxByteQty; ++i) { output.formatIdx |= (*bl++) << (8 * i); }
    _lastFormatIdx = output.formatIdx;

    output.categoryIdx = _lastCategoryIdx & (0xFFFFFFFF << (8 * categoryIdxByteQty));
    for (int i = 0; i < categoryIdxByteQty; ++i) { output.categoryIdx |= (*bl++) << (8 * i); }
    _lastCategoryIdx = output.categoryIdx;

    output.threadIdx = _lastThreadId & (0xFFFFFFFF << (8 * threadIdxByteQty));
    for (int i = 0; i < threadIdxByteQty; ++i) { output.threadIdx |= (*bl++) << (8 * i); }
    _lastThreadId = output.threadIdx;

    uint64_t timestampTick = 0;
    if (timestampByteQty < 8) { timestampTick = _lastTimestampTick & (0xFFFFFFFFFFFFFFFFULL << (8 * timestampByteQty)); }
    for (int i = 0; i < timestampByteQty; ++i) { timestampTick |= ((uint64_t)(*bl++)) << (8 * i); }
    _lastTimestampTick    = timestampTick;
    output.timestampUtcNs = _timeConverter.getUtcNs(timestampTick);

    // Find the start of the parameter values
    uint32_t valueOffset = _offset + headerSize;
    while (valueOffset < _bufferSize && (_buffer[valueOffset] & 0xF0) != 0 && (_buffer[valueOffset] & 0x0F) != 0) { ++valueOffset; }
    ++valueOffset;

    // Parse the parameters (type & value)
    output.args.clear();
    output.buffer.clear();
    int      paramQty    = 0;
    uint32_t paramOffset = _offset + headerSize;
    while (paramOffset < _bufferSize) {
        bool isLowQuartet = (paramQty & 1) == 1;  // 4 bits per parameter flag. High quartet first
        ++paramQty;
        int pflag = (_buffer[paramOffset] >> (isLowQuartet ? 0 : 4)) & 0x0F;
        if (pflag == SSLOG_DATA_NONE) { break; }
        switch (pflag) {
            case SSLOG_DATA_S8:
                if (valueOffset + 1 <= _bufferSize) {
                    output.args.push_back({ArgType::S32, 0});
                    output.args.back().vS32 = _buffer[valueOffset + 0];
                }
                valueOffset += 1;
                break;
            case SSLOG_DATA_U8:
                if (valueOffset + 1 <= _bufferSize) {
                    output.args.push_back({ArgType::U32, 0});
                    output.args.back().vU32 = _buffer[valueOffset + 0];
                }
                valueOffset += 1;
                break;
            case SSLOG_DATA_S16:
                if (valueOffset + 2 <= _bufferSize) {
                    output.args.push_back({ArgType::S32, 0});
                    output.args.back().vS32 = (_buffer[valueOffset + 0] << 0) | (_buffer[valueOffset + 1] << 8);
                }
                valueOffset += 2;
                break;
            case SSLOG_DATA_U16:
                if (valueOffset + 2 <= _bufferSize) {
                    output.args.push_back({ArgType::U32, 0});
                    output.args.back().vU32 = (_buffer[valueOffset + 0] << 0) | (_buffer[valueOffset + 1] << 8);
                }
                valueOffset += 2;
                break;
            case SSLOG_DATA_S32:
                if (valueOffset + 4 <= _bufferSize) {
                    output.args.push_back({ArgType::S32, 0});
                    output.args.back().vS32 = (_buffer[valueOffset + 0] << 0) | (_buffer[valueOffset + 1] << 8) |
                                              (_buffer[valueOffset + 2] << 16) | (_buffer[valueOffset + 3] << 24);
                }
                valueOffset += 4;
                break;
            case SSLOG_DATA_U32:
                if (valueOffset + 4 <= _bufferSize) {
                    output.args.push_back({ArgType::U32, 0});
                    output.args.back().vU32 = (_buffer[valueOffset + 0] << 0) | (_buffer[valueOffset + 1] << 8) |
                                              (_buffer[valueOffset + 2] << 16) | (_buffer[valueOffset + 3] << 24);
                }
                valueOffset += 4;
                break;
            case SSLOG_DATA_S64:
                if (valueOffset + 8 <= _bufferSize) {
                    output.args.push_back({ArgType::S64, 0});
                    output.args.back().vS64 = ((uint64_t)_buffer[valueOffset + 0] << 0) | ((uint64_t)_buffer[valueOffset + 1] << 8) |
                                              ((uint64_t)_buffer[valueOffset + 2] << 16) | ((uint64_t)_buffer[valueOffset + 3] << 24) |
                                              ((uint64_t)_buffer[valueOffset + 4] << 32) | ((uint64_t)_buffer[valueOffset + 5] << 40) |
                                              ((uint64_t)_buffer[valueOffset + 6] << 48) | ((uint64_t)_buffer[valueOffset + 7] << 56);
                }
                valueOffset += 8;
                break;
            case SSLOG_DATA_U64:
                if (valueOffset + 8 <= _bufferSize) {
                    output.args.push_back({ArgType::U64, 0});
                    output.args.back().vU64 = ((uint64_t)_buffer[valueOffset + 0] << 0) | ((uint64_t)_buffer[valueOffset + 1] << 8) |
                                              ((uint64_t)_buffer[valueOffset + 2] << 16) | ((uint64_t)_buffer[valueOffset + 3] << 24) |
                                              ((uint64_t)_buffer[valueOffset + 4] << 32) | ((uint64_t)_buffer[valueOffset + 5] << 40) |
                                              ((uint64_t)_buffer[valueOffset + 6] << 48) | ((uint64_t)_buffer[valueOffset + 7] << 56);
                }
                valueOffset += 8;
                break;
            case SSLOG_DATA_FLOAT:
                if (valueOffset + 4 <= _bufferSize) {
                    output.args.push_back({ArgType::Float, 0});
                    uint32_t tmp1 = (_buffer[valueOffset + 0] << 0) | (_buffer[valueOffset + 1] << 8) | (_buffer[valueOffset + 2] << 16) |
                                    (_buffer[valueOffset + 3] << 24);
                    char* tmp2                = (char*)&tmp1;
                    output.args.back().vFloat = *((float*)tmp2);
                }
                valueOffset += 4;
                break;
            case SSLOG_DATA_DOUBLE:
                if (valueOffset + 8 <= _bufferSize) {
                    output.args.push_back({ArgType::Double, 0});
                    uint64_t tmp1 = ((uint64_t)_buffer[valueOffset + 0] << 0) | ((uint64_t)_buffer[valueOffset + 1] << 8) |
                                    ((uint64_t)_buffer[valueOffset + 2] << 16) | ((uint64_t)_buffer[valueOffset + 3] << 24) |
                                    ((uint64_t)_buffer[valueOffset + 4] << 32) | ((uint64_t)_buffer[valueOffset + 5] << 40) |
                                    ((uint64_t)_buffer[valueOffset + 6] << 48) | ((uint64_t)_buffer[valueOffset + 7] << 56);
                    char* tmp2                 = (char*)&tmp1;
                    output.args.back().vDouble = *((double*)tmp2);
                }
                valueOffset += 8;
                break;
            case SSLOG_DATA_STRING:
                if (valueOffset + 4 <= _bufferSize) {
                    uint32_t stringIdx = (_buffer[valueOffset + 0] << 0) | (_buffer[valueOffset + 1] << 8) |
                                         (_buffer[valueOffset + 2] << 16) | (_buffer[valueOffset + 3] << 24);
                    if (stringIdx >= _session->getIndexedStringQty()) {
                        stringIdx = UINT_MAX;  // Will be displayed as "corrupted string"
                    }
                    output.args.push_back({ArgType::StringIdx, 0});
                    output.args.back().vStringIdx = stringIdx;
                }
                valueOffset += 4;
                break;
            case SSLOG_DATA_BUFFER:
                if (valueOffset + 4 <= _bufferSize) {
                    uint32_t bufferSize = (_buffer[valueOffset + 0] << 0) | (_buffer[valueOffset + 1] << 8) |
                                          (_buffer[valueOffset + 2] << 16) | (_buffer[valueOffset + 3] << 24);
                    valueOffset += 4;
                    if (valueOffset + bufferSize <= _bufferSize) {
                        output.buffer.resize(bufferSize);
                        memcpy(&output.buffer[0], &_buffer[valueOffset], bufferSize);
                    }
                    valueOffset += bufferSize;
                } else {
                    valueOffset += 4;
                }
                break;
            default:
                fprintf(stderr, "Corrupted log data: bad argument type when parsing...\n");
        }
        if (isLowQuartet) { ++paramOffset; }  // Increment after the low quartet only
    }

    // Next log
    _offset = valueOffset;
}

LogSession::Filter::Filter(const LogSession* session, const Rule& rule) : _session(session), _rule(rule)
{
    // Prepare the hash filters
    for (uint32_t l = (uint32_t)_rule.levelMin; l <= (uint32_t)_rule.levelMax; ++l) { _filterLevel[l] = true; }
    for (uint32_t stringIdx : _session->getStringIndexes(_rule.category)) { _filterCategory.insert(stringIdx + 1, true); }
    for (uint32_t stringIdx : _session->getStringIndexes(_rule.noCategory)) { _filterCategoryExcl.insert(stringIdx + 1, true); }
    for (uint32_t stringIdx : _session->getStringIndexes(_rule.thread)) { _filterThread.insert(stringIdx + 1, true); }
    for (uint32_t stringIdx : _session->getStringIndexes(_rule.noThread)) { _filterThreadExcl.insert(stringIdx + 1, true); }
    for (uint32_t stringIdx : _session->getStringIndexes(_rule.format)) { _filterFormat.insert(stringIdx + 1, true); }
    for (uint32_t stringIdx : _session->getStringIndexes(_rule.noFormat)) { _filterFormatExcl.insert(stringIdx + 1, true); }

    // Argument matching input
    for (const std::string& a : _rule.arguments) {
        ArgMultiValues mv;
        if (parseArgument(a, mv)) { _filterArgValue.push_back(mv); }
    }

    bool isPresent = false;
    if (!_filterArgValue.empty()) {
        std::vector<uint64_t> matchedArgs;
        for (uint32_t stringIdx = 0; stringIdx < _session->getIndexedStringQty(); ++stringIdx) {
            // Skip the string if it does not pass the format filtering
            if ((!_rule.format.empty() && !_filterFormat.find(stringIdx + 1, isPresent)) ||
                (!_rule.noFormat.empty() && _filterFormatExcl.find(stringIdx + 1, isPresent))) {
                continue;
            }

            // Loop on the filter arguments
            matchedArgs.clear();
            const std::vector<ArgNameAndUnit>& args = _session->getIndexedStringArgNameAndUnit(stringIdx);
            for (uint32_t filterArgMatchIdx = 0; filterArgMatchIdx < _filterArgValue.size(); ++filterArgMatchIdx) {
                // Loop on all the arguments from this string
                for (uint32_t stringArgIdx = 0; stringArgIdx < args.size(); ++stringArgIdx) {
                    if (args[stringArgIdx].name == _filterArgValue[filterArgMatchIdx].name) {
                        matchedArgs.push_back((((uint64_t)stringArgIdx) << 32) | (uint64_t)filterArgMatchIdx);  // Match
                        break;
                    }
                }
                if (matchedArgs.size() != filterArgMatchIdx + 1) { break; }
            }
            if (matchedArgs.size() != _filterArgValue.size()) { continue; }

            // The string matches the requirement of the argument filter
            _filterArgPosition.insert(stringIdx + 1, matchedArgs);
        }
    }
}

bool
LogSession::Filter::apply(const LogStream* ls) const
{
    bool             doProcess = true;
    bool             isPresent = false;
    const LogStruct& log       = ls->output;
    if (!_filterLevel[(int)log.level]) { doProcess = false; }
    if (doProcess) { doProcess = log.buffer.size() >= _rule.bufferSizeMin; }
    if (doProcess) { doProcess = log.buffer.size() <= _rule.bufferSizeMax; }
    if (doProcess && !_rule.format.empty()) { doProcess = _filterFormat.find(log.formatIdx + 1, isPresent); }
    if (doProcess && !_rule.noFormat.empty()) { doProcess = !_filterFormatExcl.find(log.formatIdx + 1, isPresent); }
    if (doProcess && !_rule.category.empty()) { doProcess = _filterCategory.find(log.categoryIdx + 1, isPresent); }
    if (doProcess && !_rule.noCategory.empty()) { doProcess = !_filterCategoryExcl.find(log.categoryIdx + 1, isPresent); }
    if (doProcess && !_rule.thread.empty()) { doProcess = _filterThread.find(log.threadIdx + 1, isPresent); }
    if (doProcess && !_rule.noThread.empty()) { doProcess = !_filterThreadExcl.find(log.threadIdx + 1, isPresent); }
    if (doProcess && !_filterArgValue.empty()) {  // Argument filtering is more complex due to the 2 levels check and the typing
        std::vector<uint64_t> result;
        doProcess = _filterArgPosition.find(log.formatIdx + 1, result);
        if (doProcess) {
            // Loop on arguments to match
            for (uint64_t rawIdx : result) {
                uint32_t stringArgIdx = (rawIdx >> 32) & 0xFFFFFFFF;
                if (stringArgIdx >= log.args.size()) {
                    doProcess = false;  // Not enough param provided during logging for this format string
                } else {
                    // Get the argument value from the log
                    const ArgMultiValues& mv      = _filterArgValue[rawIdx & 0xFFFFFFFF];
                    const Arg&            p       = log.args[stringArgIdx];
                    int64_t               vS64    = 0;
                    uint64_t              vU64    = 0;
                    double                vDouble = 0.;
                    const char*           vString = nullptr;
                    switch (p.pType) {
                        case ArgType::S32:
                            vS64 = p.vS32;
                            break;
                        case ArgType::S64:
                            vS64 = p.vS64;
                            break;
                        case ArgType::U32:
                            vU64 = p.vU32;
                            break;
                        case ArgType::U64:
                            vU64 = p.vU64;
                            break;
                        case ArgType::Float:
                            vDouble = p.vFloat;
                            break;
                        case ArgType::Double:
                            vDouble = p.vDouble;
                            break;
                        case ArgType::StringIdx:
                            vString = _session->getIndexedString(p.vStringIdx);
                            break;
                    };
                    // Match it
                    doProcess = true;
                    if (mv.checkEqual || mv.checkGreater || mv.checkLesser) {
                        switch (p.pType) {
                            case ArgType::S32:
                            case ArgType::S64:
                                doProcess = mv.hasNumeric && ((mv.checkEqual && vS64 == mv.vS64) || (mv.checkGreater && vS64 > mv.vS64) ||
                                                              (mv.checkLesser && vS64 < mv.vS64));
                                break;
                            case ArgType::U32:
                            case ArgType::U64:
                                doProcess = mv.hasNumeric && ((mv.checkEqual && vU64 == mv.vU64) || (mv.checkGreater && vU64 > mv.vU64) ||
                                                              (mv.checkLesser && vU64 < mv.vU64));
                                break;
                            case ArgType::Float:
                            case ArgType::Double:
                                doProcess = mv.hasNumeric &&
                                            ((mv.checkEqual && vDouble == mv.vDouble) || (mv.checkGreater && vDouble > mv.vDouble) ||
                                             (mv.checkLesser && vDouble < mv.vDouble));
                                break;
                            case ArgType::StringIdx:
                                doProcess = (mv.checkEqual && strcmp(vString, mv.vString.c_str()) == 0) ||
                                            (mv.checkGreater && strcmp(vString, mv.vString.c_str()) > 0) ||
                                            (mv.checkLesser && strcmp(vString, mv.vString.c_str()) < 0);
                                break;
                        };
                    }
                    if (!doProcess) { break; }
                }
            }
        }
    }

    return doProcess;
}

bool
LogSession::Filter::parseArgument(const std::string& a, ArgMultiValues& mv)
{
    mv = {};

    // Name
    const char* n = a.c_str();
    while (*n != 0 && *n != '=' && *n != '>' && *n != '<') ++n;
    if (n == a.c_str()) return false;
    mv.name = std::string(a.c_str(), n);
    if (*n == 0) {
        return true;  // Name without operation nor value. Simple check for existence.
    }

    // Operator
    const char* o = n;
    while (*o != 0 && (*o == '=' || *o == '>' || *o == '<')) ++o;
    assert(o != n);
    std::string argOp = std::string(n, o);
    if (argOp == "=" || argOp == "==")
        mv.checkEqual = true;
    else if (argOp == ">") {
        mv.checkGreater = true;
    } else if (argOp == ">=") {
        mv.checkGreater = true;
        mv.checkEqual   = true;
    } else if (argOp == "<") {
        mv.checkLesser = true;
    } else if (argOp == "<=") {
        mv.checkLesser = true;
        mv.checkEqual  = true;
    }

    // Value
    const char* v = o;
    while (*v != 0) ++v;
    mv.vString = std::string(o, v);
    if (mv.vString.empty()) {
        mv.checkEqual = mv.checkGreater = mv.checkLesser = false;
        return true;  // Slightly incorrect syntax with an operator but no value. Fallback to an existence check.
    }
    char* pEnd    = nullptr;
    mv.vS64       = strtoll(o, &pEnd, 0);
    mv.hasNumeric = (pEnd != o);
    mv.vU64       = strtoull(o, &pEnd, 0);
    mv.hasNumeric = mv.hasNumeric && (pEnd != o);
    mv.vDouble    = strtod(o, &pEnd);
    mv.hasNumeric = mv.hasNumeric && (pEnd != o);
    return true;
}

}  // namespace sslogread
