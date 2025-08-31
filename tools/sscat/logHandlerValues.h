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

#include <string>

#include "sslogread/sslogread.h"

class LogHandlerValues
{
   public:
    LogHandlerValues(const sslogread::LogSession& session, const std::string& dateFormatterString, bool withUtcTime,
                     const std::vector<std::string>& argNames, const std::string& separator);
    ~LogHandlerValues() = default;

    void notifyLog(const sslogread::LogStruct& log);

   private:
    const sslogread::LogSession&   _session;
    const std::vector<std::string> _argNames;
    const std::string              _separator;
    bool                           _isFirstDisplayed = true;
    char                           _filledFormatbuffer[8192];
    char                           _formattedDate[128];
    sslog::priv::TextFormatter     _tf;
};
