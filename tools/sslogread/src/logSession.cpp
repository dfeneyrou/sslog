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

#include <algorithm>

#if WITH_ZSTD
#include <zstd.h>  // presumes zstd library is installed
#endif

#include "sslogread/sslogread.h"

namespace sslogread
{

// C++17 strings are not really helpful...
static bool
ends_with(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool
LogSession::loadUncompressedFile(const std::filesystem::path& filePath, std::string& errorMessage, std::vector<uint8_t>& buffer) const
{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)  // Disable Windows's secure API warnings on fopen
#endif
    FILE* fileHandle = nullptr;
    if ((fileHandle = fopen(filePath.string().c_str(), "rb")) == nullptr) {
        errorMessage = std::string("unable to open the file '") + filePath.string() + "' for reading";
        return false;
    }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

    // Get the file size
    fseek(fileHandle, 0L, SEEK_END);
    size_t fileSize = ftell(fileHandle);
    fseek(fileHandle, 0L, SEEK_SET);
    if (fileSize > (1ULL << 32)) {
        errorMessage = std::string("the file is too large to be valid (") + std::to_string(fileSize) + " bytes)";
        fclose(fileHandle);
        return false;
    }

    // Read the full file
    buffer.resize(fileSize);
    if (fread((void*)buffer.data(), 1, fileSize, fileHandle) != fileSize) {
        errorMessage = std::string("unable to read the file ") + filePath.string();
        fclose(fileHandle);
        return false;
    }

    if (_doSumFileStatSizes) { _statLogBytes += fileSize; }

    fclose(fileHandle);
    return true;
}

bool
LogSession::loadCompressedFile(const std::filesystem::path& filePath, std::string& errorMessage, std::vector<uint8_t>& buffer) const
{
#if WITH_ZSTD
    // Load the compressed file
    std::vector<uint8_t> compressedBuffer;
    if (!loadUncompressedFile(filePath, errorMessage, compressedBuffer)) { return false; }
    // Check the header
    uint64_t rSize = ZSTD_getFrameContentSize(compressedBuffer.data(), compressedBuffer.size());
    if (rSize == ZSTD_CONTENTSIZE_ERROR) {
        errorMessage = filePath.string() + std::string("was not compressed by zstd");
        return false;
    }
    if (rSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        errorMessage = filePath.string() + std::string("does not provide its uncompressed size");
        return false;
    }
    buffer.resize(rSize);

    // Decompress
    size_t dSizeOrErr = ZSTD_decompress(buffer.data(), buffer.size(), compressedBuffer.data(), compressedBuffer.size());
    if (ZSTD_isError(dSizeOrErr)) {
        errorMessage = ZSTD_getErrorName(dSizeOrErr);
        return false;
    }
    return true;
#else
    (void)filePath;
    (void)buffer;
    errorMessage = "Zstandard libraries not included in sslogread, unable to decompress the log file";
    return false;
#endif
}

bool
LogSession::readBaseInfos(const std::filesystem::path& logDirPath, std::string& errorMessage)
{
    errorMessage.clear();

    // Open the log files
    if (!std::filesystem::exists(logDirPath)) {
        errorMessage = std::string("the directory '") + logDirPath.string() + "' does not exist";
        return false;
    }
    if (!std::filesystem::is_directory(logDirPath)) {
        errorMessage = std::string("'") + logDirPath.string() + "' must be a directory";
        return false;
    }

    _statLogBytes       = 0;
    _doSumFileStatSizes = true;
    std::vector<uint8_t> stringBuffer;
    if (std::filesystem::exists(logDirPath / "base.sslog")) {
        if (!loadUncompressedFile(logDirPath / "base.sslog", errorMessage, stringBuffer)) { return false; }
    } else if (std::filesystem::exists(logDirPath / "base.sslog.zst")) {
        if (!loadCompressedFile(logDirPath / "base.sslog.zst", errorMessage, stringBuffer)) { return false; }
    } else {
        errorMessage = std::string("the file '") + (logDirPath / "base.sslog").string() + "' does not exist";
        return false;
    }
    _doSumFileStatSizes = false;
    _statBaseBytes      = _statLogBytes;
    _statLogBytes       = 0;

    // Parse the header
    uint8_t* header = stringBuffer.data();
    int      offset = 0;
    if (strncmp((char*)&header[offset], "SSSSSS", 6) != 0) {
        errorMessage = "this is not a sslog file type";
        return false;
    }
    offset += 6;
    _formatVersion = (header[offset + 0] << 0) | (header[offset + 1] << 8);
    if (_formatVersion != 1) {
        errorMessage = std::string("this storage format version (") + std::to_string(_formatVersion) + ") is not supported";
        return false;
    }
    offset += 2;
    _sessionId = 0;
    for (size_t i = 0; i < 8; ++i) { _sessionId |= ((uint64_t)header[offset + i] << (8 * i)); }
    offset += 8;
    _utcSystemClockStartNs = 0;
    for (size_t i = 0; i < 8; ++i) { _utcSystemClockStartNs |= ((uint64_t)header[offset + i] << (8 * i)); }
    offset += 8;
    uint64_t tmpHighResTickToNs = 0;
    for (size_t i = 0; i < 8; ++i) { tmpHighResTickToNs |= ((uint64_t)header[offset + i] << (8 * i)); }
    char* tmp2HighResTickToNs = (char*)&tmpHighResTickToNs;
    _tickToNs                 = *((double*)tmp2HighResTickToNs);
    offset += 8;
    assert(offset == BaseHeaderSize);

    // Remove the header
    memmove(header, header + BaseHeaderSize, stringBuffer.size() - BaseHeaderSize);
    stringBuffer.resize(stringBuffer.size() - BaseHeaderSize);
    setStringBuffer(std::move(stringBuffer));

    return true;
}

bool
LogSession::readDataInfos(const std::filesystem::path& dataFilePath, DataInfos& infos, std::string& errorMessage) const
{
    errorMessage.clear();

    // Open the log files
    if (!std::filesystem::exists(dataFilePath)) {
        errorMessage = std::string("the file '") + dataFilePath.string() + "' does not exist";
        return false;
    }

    if (dataFilePath.extension().string() == ".zst") {
        if (!loadCompressedFile(dataFilePath, errorMessage, infos.logBuffer)) { return false; }
    } else {
        if (!loadUncompressedFile(dataFilePath, errorMessage, infos.logBuffer)) { return false; }
    }

    // Parse the header
    uint8_t* header = infos.logBuffer.data();
    int      offset = 0;
    if (strncmp((char*)&header[offset], "SSSSSS", 6) != 0) {
        errorMessage = "Error: this is not a sslog file type";
        return false;
    }
    offset += 6;
    infos.formatVersion = (header[offset + 0] << 0) | (header[offset + 1] << 8);
    if (infos.formatVersion != 1) {
        errorMessage = std::string("Error: this storage format version (") + std::to_string(_formatVersion) + ") is not supported";
        return false;
    }
    offset += 2;
    infos.sessionId = 0;
    for (size_t i = 0; i < 8; ++i) { infos.sessionId |= ((uint64_t)header[offset + i] << (8 * i)); }
    offset += 8;
    infos.utcSystemClockOriginNs = 0;
    for (size_t i = 0; i < 8; ++i) { infos.utcSystemClockOriginNs |= ((uint64_t)header[offset + i] << (8 * i)); }
    offset += 8;
    infos.steadyClockOriginTick = 0;
    for (size_t i = 0; i < 8; ++i) { infos.steadyClockOriginTick |= ((uint64_t)header[offset + i] << (8 * i)); }
    offset += 8;
    assert(offset == DataHeaderSize);

    // Remove the header
    memmove(header, header + DataHeaderSize, infos.logBuffer.size() - DataHeaderSize);
    infos.logBuffer.resize(infos.logBuffer.size() - DataHeaderSize);
    return true;
}

bool
LogSession::init(const std::filesystem::path& logDirPath, std::string& errorMessage)
{
    // Read the meta information and strings from the base file
    if (!readBaseInfos(logDirPath, errorMessage)) { return false; }

    // Complete the session with the data files information
    std::error_code                     ec;
    std::filesystem::directory_iterator di(logDirPath, ec);
    if (ec) {
        errorMessage = std::string("sslogread error: unable to access the directory '") + logDirPath.string() + "': " + ec.message();
        return false;
    }

    for (auto const& de : di) {
        auto f = de.path().filename().string();
        if (de.is_regular_file() && strncmp("data", f.c_str(), 4) == 0 && (ends_with(f, ".sslog") || ends_with(f, ".sslog.zst")) &&
            (!ends_with(f, ".zst") || !std::filesystem::exists(f.substr(0, f.size() - 4)))) {  // Select uncompressed over compressed
            _dataFileList.push_back(de.path().string());
        }
    }
    std::sort(_dataFileList.begin(), _dataFileList.end());  // Sort by number
    return true;
}

bool
LogSession::query(const std::vector<Rule>& rules, const std::function<void(int, const LogStruct&)>& callback,
                  std::string& errorMessage) const
{
    // Prepare the filters (OR between them)
    std::vector<std::unique_ptr<Filter>> filters;
    filters.reserve(rules.size());
    for (const Rule& rule : rules) { filters.push_back(std::make_unique<Filter>(this, rule)); }

    sslog::priv::TimeConverter timeConverter;
    timeConverter.init(_tickToNs, 0, 0);

    // Loop on ordered files
    for (uint32_t fileIdx = 0; fileIdx < _dataFileList.size(); ++fileIdx) {
        // Detailed data file comes first in sorting.
        // If it is followed by a standard data file with the same number, process them together and interlaced
        const std::string& dataFilename1 = _dataFileList[fileIdx];
        std::string        dataFilename2;
        if (fileIdx + 1 < _dataFileList.size() && ends_with(dataFilename1, ".dtl.sslog") &&
            dataFilename1.substr(0, dataFilename1.size() - 10) ==
                _dataFileList[fileIdx + 1].substr(0, _dataFileList[fileIdx + 1].size() - 6)) {
            dataFilename2 = _dataFileList[++fileIdx];
        }

        // Read the data information
        DataInfos dataInfos1, dataInfos2;
        if (!readDataInfos(dataFilename1, dataInfos1, errorMessage)) { return false; }
        if (!dataFilename2.empty() && !readDataInfos(dataFilename2, dataInfos2, errorMessage)) { return false; }

        if (dataInfos1.sessionId != _sessionId || (!dataFilename2.empty() && dataInfos2.sessionId != _sessionId)) {
            continue;  // Sessions ID do not match, skip this file
        }

        timeConverter.updateSync(dataInfos1.utcSystemClockOriginNs, dataInfos1.steadyClockOriginTick);
        LogStream data1(this, dataInfos1, timeConverter);
        LogStream data2(this, dataInfos2, timeConverter);

        // Parse and format
        while (!data1.empty() || !data2.empty()) {
            // Find the next stream to consider (which did not reach its end or with the earliest next event)
            bool       doChooseSecond = (data1.empty() || (!data2.empty() && data1.output.timestampUtcNs > data2.output.timestampUtcNs));
            LogStream* ls             = doChooseSecond ? &data2 : &data1;

            // Filter
            bool doProcess = filters.empty();
            int  ruleIdx   = 0;
            while (ruleIdx < (int)filters.size()) {
                if (filters[ruleIdx]->apply(ls)) {
                    doProcess = true;
                    break;
                }
                ++ruleIdx;
            }

            if (doProcess) { callback(ruleIdx, ls->output); }

            ls->readNext();
        }

    }  // End of loop on ordered files
    return true;
}

}  // namespace sslogread
