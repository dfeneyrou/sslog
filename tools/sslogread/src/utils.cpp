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

#include "sslogread/utils.h"

namespace sslogread
{

// Inspired from https://github.com/zhicheng/base64/blob/master/base64.c (public domain)
void
base64Encode(const std::vector<uint8_t>& bufIn, std::vector<char>& bufOut)
{
    constexpr uint8_t dict[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
        'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
        's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
    };

    bufOut.clear();
    int     s = 0;
    uint8_t l = 0;
    for (uint8_t c : bufIn) {
        switch (s) {
            case 0:
                s = 1;
                bufOut.push_back(dict[(c >> 2) & 0x3F]);
                break;
            case 1:
                s = 2;
                bufOut.push_back(dict[((l & 0x3) << 4) | ((c >> 4) & 0xF)]);
                break;
            case 2:
                s = 0;
                bufOut.push_back(dict[((l & 0xF) << 2) | ((c >> 6) & 0x3)]);
                bufOut.push_back(dict[c & 0x3F]);
                break;
        }
        l = c;
    }
    switch (s) {
        case 1:
            bufOut.push_back(dict[(l & 0x3) << 4]);
            bufOut.push_back('=');
            bufOut.push_back('=');
            break;
        case 2:
            bufOut.push_back(dict[(l & 0xF) << 2]);
            bufOut.push_back('=');
            break;
    }
    bufOut.push_back(0);
}

std::vector<PatternChunk>
getPatternChunks(const std::string& pattern)
{
    std::vector<PatternChunk> patternChunks;
    const char*               p = pattern.c_str();
    while (*p != 0) {
        bool doForwardSearch = (*p == '*');
        while (doForwardSearch && *p == '*') ++p;
        // Look for the string to find
        const char* pe = p;
        while (*pe != 0 && *pe != '*') ++pe;
        patternChunks.push_back({doForwardSearch, std::string(p, pe)});
        p = pe;
    }
    if (patternChunks.empty()) { patternChunks.push_back({true, ""}); }
    return patternChunks;
}

bool
isStringMatching(const std::vector<PatternChunk>& patternChunks, const char* stringToMatch)
{
    assert(!patternChunks.empty());  // By construction in the function above

    struct SearchHypothesis {
        const char* s;
        uint32_t    chunkIdx;
    };

    std::vector<const char*>      matchResults;
    std::vector<SearchHypothesis> hyps;
    hyps.push_back({stringToMatch, 0});
    bool isMatching = false;

    while (!hyps.empty() && !isMatching) {
        SearchHypothesis h = hyps.back();
        hyps.pop_back();
        const PatternChunk& p = patternChunks[h.chunkIdx];

        // Match the pattern chunk
        matchResults.clear();
        if (p.searchString.empty()) {
            const char* se = h.s;
            while (*se) ++se;  // Search an empty pattern go to the end of the line
            matchResults.push_back(se);
        } else {
            const char* se = h.s;
            while ((se = strstr(se, p.searchString.c_str())) != nullptr) {
                matchResults.push_back(se++);
                if (!p.doForwardSearch) break;
            }
        }

        // Check all matches
        ++h.chunkIdx;  // Point on next chunk
        for (const char* se : matchResults) {
            if (!p.doForwardSearch && se != h.s) {  // Not a match as the pattern chunk was not found
                continue;
            }
            h.s = se + p.searchString.size();
            if (h.chunkIdx < patternChunks.size()) {
                hyps.push_back(h);   // Process next chunk
            } else if (*h.s == 0) {  // Both pattern and string are consumed?
                hyps.push_back(h);
                isMatching = true;
                break;
            }
        }
    }

    isMatching = (!hyps.empty() && *hyps.back().s == 0);  // The last hyp goes to the end
    return isMatching;
}

}  // namespace sslogread
