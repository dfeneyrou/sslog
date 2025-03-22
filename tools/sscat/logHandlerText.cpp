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

#include "logHandlerText.h"

#include "sslogread/utils.h"

LogHandlerText::LogHandlerText(const sslogread::LogSession& session, const std::string& formatterString, bool withUtcTime, bool withColor)
    : _session(session)
{
    _tf.init(formatterString.c_str(), withUtcTime, withColor);
}

void
LogHandlerText::notifyLog(const sslogread::LogStruct& log)
{
    char logBuffer[8192];

    sslogread::vsnprintfLog(_filledFormatbuffer, sizeof(_filledFormatbuffer), _session.getIndexedString(log.formatIdx), log.args,
                            &_session);
    _tf.format(logBuffer, sizeof(logBuffer), log.timestampUtcNs, (int)log.level, _session.getIndexedString(log.threadIdx),
               _session.getIndexedString(log.categoryIdx), _filledFormatbuffer, log.buffer.data(), (uint32_t)log.buffer.size(), true);

    // fwrite on stdout is much faster than printf with non-formatting strings
    fwrite(logBuffer, 1, strlen(logBuffer), stdout);
}
