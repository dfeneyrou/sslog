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
#include <vector>

#include "sslogread.h"

namespace sslogread
{

void
base64Encode(const std::vector<uint8_t>& bufIn, std::vector<char>& bufOut);

// Custom implementation of vsnprintf with replaced va_list
// If valuePositions is not null, it will be filled with the positions of the written values
int
vsnprintfLog(char* buf, int count, char const* fmt, const std::vector<Arg>& va, const LogSession* session,
             std::vector<int>* valuePositions = nullptr);

// Pattern matching functions
struct PatternChunk {
    bool        doForwardSearch = false;
    std::string searchString;
};

// Pre-chew the pattern into chunks (for performance reasons)
std::vector<PatternChunk>
getPatternChunks(const std::string& pattern);

// Test the string against the chewed pattern
bool
isStringMatching(const std::vector<PatternChunk>& patternChunks, const char* stringToMatch);

}  // namespace sslogread
