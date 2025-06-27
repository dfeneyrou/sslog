// System
#include <cstring>

// Internal
#include "bsString.h"

// Character conversion functions

uint8_t*
bsCharUtf8ToUnicode(uint8_t* begin, uint8_t* end, char16_t& codepoint)
{
    codepoint         = 0;
    int trailingBytes = 0;
    if (((*begin) & 0x80) == 0x00)
        trailingBytes = 0;
    else if (((*begin) & 0xE0) == 0xC0)
        trailingBytes = 1;
    else if (((*begin) & 0xF0) == 0xE0)
        trailingBytes = 2;
    else
        return nullptr;                                // Failure, unable to support 32 bits unicode, only 16 bits
    if (begin + trailingBytes >= end) return nullptr;  // Failure due to corrupted input

    uint32_t output = 0;
    switch (trailingBytes) {
        case 2:
            output += *begin++;
            output <<= 6;  // fall through
        case 1:
            output += *begin++;
            output <<= 6;  // fall through
        case 0:
            output += *begin++;
    }
    static const uint32_t offsetPerTrailingByte[6] = {0x0, 0x3080, 0xE2080};
    codepoint                                      = (char16_t)(output - offsetPerTrailingByte[trailingBytes]);
    return begin;
}

bool
bsCharUnicodeToUtf8(char16_t codepoint, bsString& outUtf8)
{
    if ((codepoint >= 0xD800 && codepoint <= 0xDBFF)) return false;

    int outSize = 3;
    if (codepoint < 0x80)
        outSize = 1;
    else if (codepoint < 0x800)
        outSize = 2;

    // Extract the bytes to write
    static const uint8_t firstBytes[4] = {0x00, 0x00, 0xC0, 0xE0};
    int                  curSize       = outUtf8.size();
    outUtf8.resize(curSize + outSize);
    switch (outSize) {
        case 3:
            outUtf8[curSize + 2] = (codepoint | 0x80) & 0xBF;
            codepoint >>= 6;  // fall through
        case 2:
            outUtf8[curSize + 1] = (codepoint | 0x80) & 0xBF;
            codepoint >>= 6;  // fall through
        case 1:
            outUtf8[curSize + 0] = (uint8_t)(codepoint | firstBytes[outSize]);
    }
    return true;
}

// UTF-8 String class
bsString::bsString(const char* s)
{
    uint8_t* s1 = (uint8_t*)s;
    while (*s1) push_back(*s1++);
}

bsString
bsString::operator+(const char* s) const
{
    uint8_t* s1 = (uint8_t*)s;
    bsString other(*this);
    while (*s1) other.push_back(*s1++);
    return other;
}

bsString
bsString::operator+(const bsString& s) const
{
    bsString other(*this);
    for (uint8_t e : s) other.push_back(e);
    return other;
}

bsStringUtf16
bsString::toUtf16() const
{
    bsStringUtf16 outUtf16;
    outUtf16.reserve(size());
    uint8_t* start = _array;
    char16_t c     = 0;
    while ((start = bsCharUtf8ToUnicode(start, _array + size(), c)) != nullptr) outUtf16.push_back(c);
    return outUtf16;
}

bsString&
bsString::operator+=(const char* s)
{
    uint8_t* s1 = (uint8_t*)s;
    while (*s1) push_back(*s1++);
    return *this;
}

bool
bsString::startsWith(const bsString& prefix) const
{
    if (prefix.size() > size()) return false;
    for (int i = 0; i < prefix.size(); ++i) {
        if (_array[i] != prefix[i]) return false;
    }
    return true;
}

bool
bsString::endsWith(const bsString& suffix, int suffixStartIdx) const
{
    if (suffixStartIdx < 0) suffixStartIdx = 0;
    int offset = size() - suffix.size();
    if (offset + suffixStartIdx < 0) return false;
    for (int i = suffix.size() - 1; i >= suffixStartIdx; --i) {
        if (_array[offset + i] != suffix[i]) return false;
    }
    return true;
}

int
bsString::findChar(char c) const
{
    for (int i = 0; i < size(); ++i) {
        if (_array[i] == (uint8_t)c) return i;
    }
    return -1;
}

int
bsString::rfindChar(char c) const
{
    for (int i = size() - 1; i >= 0; --i) {
        if (_array[i] == (uint8_t)c) return i;
    }
    return -1;
}

bsString
bsString::subString(int startIdx, int endIdx) const
{
    if (startIdx < 0) startIdx = 0;
    if (endIdx > size() || endIdx < 0) endIdx = size();
    if (startIdx >= endIdx) return bsString();
    return bsString((char*)_array + startIdx, (char*)_array + endIdx);
}

bsString
bsString::capitalize() const
{
    bsString copy(*this);
    if (!copy.empty() && copy._array[0] >= 'a' && copy._array[0] <= 'z') copy._array[0] += (uint8_t)('A' - 'a');
    return copy;
}

bsString&
bsString::strip()
{
    while (!empty() && (back() == 0 || back() == ' ')) pop_back();
    int firstNonSpaceIdx = 0;
    while (firstNonSpaceIdx < size() && _array[firstNonSpaceIdx] == ' ') ++firstNonSpaceIdx;
    if (firstNonSpaceIdx == 0) return *this;
    memmove(&_array[0], &_array[firstNonSpaceIdx], size() - firstNonSpaceIdx);
    resize(size() - firstNonSpaceIdx);
    return *this;
}

bsString&
bsString::filterForFilename()
{
    // Filter out the usual problematic characters for file systems
    int i = 0;
    for (uint8_t c : *this) {
        if (c < 0x1F || c == 0x7F || c == '"' || c == '*' || c == '/' || c == '\\' || c == ':' || c == '<' || c == '>' || c == '^' ||
            c == '?' || c == '|')
            continue;
        (*this)[i++] = c;
    }
    resize(i);
    strip();
    return *this;
}

bsString&
bsString::operator+=(const bsString& s)
{
    reserve(size() + s.size());
    for (uint8_t e : s) push_back(e);
    return *this;
}

// UTF-16 String class

bsStringUtf16::bsStringUtf16(const char16_t* s)
{
    char16_t* s1 = (char16_t*)s;
    while (*s1) push_back(*s1++);
}

bsStringUtf16
bsStringUtf16::operator+(const char* s) const
{
    bsStringUtf16 other(*this);
    char*         s1 = (char*)s;
    while (*s1) other.push_back((char16_t)(*s1++));
    return other;
}

bsStringUtf16
bsStringUtf16::operator+(const bsStringUtf16& s) const
{
    bsStringUtf16 other(*this);
    for (char16_t e : s) other.push_back(e);
    return other;
}

bsStringUtf16&
bsStringUtf16::operator+=(const char* s)
{
    char* s1 = (char*)s;
    while (*s1) push_back((char16_t)(*s1++));
    return *this;
}

bsStringUtf16&
bsStringUtf16::operator+=(const bsStringUtf16& s)
{
    for (char16_t e : s) push_back(e);
    return *this;
}

bsString
bsStringUtf16::toUtf8() const
{
    bsString outUtf8;
    outUtf8.reserve(size());
    for (char16_t e : *this) bsCharUnicodeToUtf8(e, outUtf8);
    return outUtf8;
}
