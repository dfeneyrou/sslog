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

#include "logHandlerValues.h"

#include "sslogread/utils.h"

LogHandlerValues::LogHandlerValues(const sslogread::LogSession& session, const std::string& dateFormatterString, bool withUtcTime,
                                   const std::vector<std::string>& argNames, const std::string& separator)
    : _session(session), _argNames(argNames), _separator(separator)
{
    _tf.init(dateFormatterString.c_str(), withUtcTime, false, session.getUtcSystemClockOriginNs());
}

void
LogHandlerValues::notifyLog(const sslogread::LogStruct& log)
{
    const std::vector<sslogread::ArgNameAndUnit>& argSpecs = _session.getIndexedStringArgNameAndUnit(log.formatIdx);

    bool isFirst = true;
    for (const std::string& targetName : _argNames) {
        // Empty arg name means "timestamp"
        if (targetName.empty()) {
            if (!isFirst) { printf("%s", _separator.c_str()); }
            isFirst = false;
            _tf.format(_formattedDate, sizeof(_formattedDate), log.timestampUtcNs, SSLOG_LEVEL_QTY, "", "", "", nullptr, 0, false);
            printf("%s", _formattedDate);
            continue;
        }

        for (uint32_t argIdx = 0; argIdx < log.args.size() && argIdx < argSpecs.size(); ++argIdx) {
            if (argSpecs[argIdx].name != targetName) { continue; }
            const sslogread::Arg& p = log.args[argIdx];
            if (!isFirst) { printf("%s", _separator.c_str()); }
            isFirst = false;

            switch (p.pType) {
                case sslogread::ArgType::S32:
                    printf("%d", p.vS32);
                    break;
                case sslogread::ArgType::U32:
                    printf("%u", p.vU32);
                    break;
                case sslogread::ArgType::S64:
                    printf("%" PRId64, p.vS64);
                    break;
                case sslogread::ArgType::U64:
                    printf("%" PRIu64, p.vU64);
                    break;
                case sslogread::ArgType::Float:
                    printf("%f", p.vFloat);
                    break;
                case sslogread::ArgType::Double:
                    printf("%f", p.vDouble);
                    break;
                case sslogread::ArgType::StringIdx:
                    printf("%s", _session.getIndexedString(p.vStringIdx));
                    break;
            };

            break;
        }  // End of loop on log names
    }  // End of loop on target names

    if (!isFirst) { printf("\n"); }
    fflush(stdout);
}
