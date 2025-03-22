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

#include "logHandlerJson.h"

#include "sslogread/utils.h"

// Helpers
// =======

static const char*
jsonEscape(const char* s)
{
    static std::vector<char> buf;  // Working buffer

    // Quick Check that escaping is required (usually not)
    // Targets are multiline and double quote
    const char* s1 = s;
    while (*s1 != 0 && *s1 != '\n' && *s1 != '"') ++s1;
    if (*s1 == 0) return s;

    buf.clear();
    while (*s != 0) {
        const char* s2 = s;
        while (*s2 != 0 && *s2 != '\n' && *s2 != '"') ++s2;
        if (s2 != s) {
            int offset = (int)buf.size();
            buf.resize(offset + s2 - s);
            memcpy(&buf[offset], s, s2 - s);
            s = s2;
        }
        if (*s == '"') {
            int offset = (int)buf.size();
            buf.resize(offset + 2);
            buf[offset]     = '\\';
            buf[offset + 1] = '"';
            ++s;
        }
        if (*s == '\n') {
            int offset = (int)buf.size();
            buf.resize(offset + 2);
            buf[offset]     = '\\';
            buf[offset + 1] = 'n';
            ++s;
        }
    }

    return &buf[0];
}

LogHandlerJson::LogHandlerJson(const sslogread::LogSession& session, const std::string& formatterString, bool withUtcTime)
    : _session(session)
{
    _tf.init(formatterString.c_str(), withUtcTime, false);
    printf("{\n  \"logs\" : [\n");
}

LogHandlerJson::~LogHandlerJson() { printf("  ]\n} \n"); }

void
LogHandlerJson::notifyLog(const sslogread::LogStruct& log)
{
    const std::vector<sslogread::ArgNameAndUnit>& args = _session.getIndexedStringArgNameAndUnit(log.formatIdx);
    if (!_isFirstDisplayed) { printf(",\n"); }
    _isFirstDisplayed = false;
    sslogread::vsnprintfLog(_filledFormatbuffer, sizeof(_filledFormatbuffer), _session.getIndexedString(log.formatIdx), log.args,
                            &_session);
    _tf.format(_formattedDate, sizeof(_formattedDate), log.timestampUtcNs, SSLOG_LEVEL_QTY, "", "", "", nullptr, 0, false);
    printf(
        "    { \"timestamp\":  \"%s\", \"thread\": \"%s\", \"category\": \"%s\", \"level\": \"%s\", \"format\": \"%s\",\n"
        "      \"args\": [",
        _formattedDate, jsonEscape(_session.getIndexedString(log.threadIdx)), jsonEscape(_session.getIndexedString(log.categoryIdx)),
        sslogread::LogSession::getLevelName(sslog::Level(log.level)), jsonEscape(_filledFormatbuffer));
    for (uint32_t argIdx = 0; argIdx < log.args.size() && argIdx < args.size(); ++argIdx) {
        const sslogread::Arg& p = log.args[argIdx];
        if (argIdx > 0) printf(",");
        switch (p.pType) {
            case sslogread::ArgType::S32:
                printf(" { \"name\": \"%s\", \"unit\": \"%s\", \"type\": \"s32\", \"value\": %d }", args[argIdx].name.c_str(),
                       args[argIdx].unit.c_str(), p.vS32);
                break;
            case sslogread::ArgType::U32:
                printf(" { \"name\": \"%s\", \"unit\": \"%s\", \"type\": \"u32\", \"value\": %u }", args[argIdx].name.c_str(),
                       args[argIdx].unit.c_str(), p.vU32);
                break;
            case sslogread::ArgType::S64:
                printf(" { \"name\": \"%s\", \"unit\": \"%s\", \"type\": \"s64\", \"value\": %" PRId64 " }", args[argIdx].name.c_str(),
                       args[argIdx].unit.c_str(), p.vS64);
                break;
            case sslogread::ArgType::U64:
                printf(" { \"name\": \"%s\", \"unit\": \"%s\", \"type\": \"u64\", \"value\": %" PRIu64 " }", args[argIdx].name.c_str(),
                       args[argIdx].unit.c_str(), p.vU64);
                break;
            case sslogread::ArgType::Float:
                printf(" { \"name\": \"%s\", \"unit\": \"%s\", \"type\": \"f32\", \"value\": %f }", args[argIdx].name.c_str(),
                       args[argIdx].unit.c_str(), p.vFloat);
                break;
            case sslogread::ArgType::Double:
                printf(" { \"name\": \"%s\", \"unit\": \"%s\", \"type\": \"f64\", \"value\": %f }", args[argIdx].name.c_str(),
                       args[argIdx].unit.c_str(), p.vDouble);
                break;
            case sslogread::ArgType::StringIdx:
                printf(" { \"name\": \"%s\", \"unit\": \"%s\", \"type\": \"str\", \"value\": \"%s\" }",
                       jsonEscape(args[argIdx].name.c_str()), jsonEscape(args[argIdx].unit.c_str()),
                       jsonEscape(_session.getIndexedString(p.vStringIdx)));
                break;
        };
    }
    if (!log.buffer.empty()) {
        if (log.args.size() > 0) printf(",");
        sslogread::base64Encode(log.buffer, _base64Output);
        printf(" { \"name\": \"\", \"unit\": \"base64\", \"type\": \"buffer\", \"value\": \"%s\" }", (const char*)&_base64Output[0]);
    }
    printf(" ] }");
    fflush(stdout);
}
