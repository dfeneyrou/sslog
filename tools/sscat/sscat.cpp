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

// This file is a utility tool to dump the logs

#include <time.h>

#include <cassert>
#include <cinttypes>  // Platform independent printf for integers (PRI64...)
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#include "windows.h"
#endif  // _WIN32

#define SSLOG_NO_AUTOSTART 1
#include "logHandlerJson.h"
#include "logHandlerText.h"
#include "logHandlerValues.h"
#include "sslog.h"
#include "sslogread/sslogread.h"

using namespace sslogread;

// =========================
// Main
// =========================

enum class Mode { Help, ColorText, MonoText, Json, Infos, Values };

bool
parseParameters(int argc, char** argv, std::filesystem::path& logDirPath, Mode& mode, bool& withUtcTime, std::vector<Rule>& rules,
                std::string& consoleFormatter, std::string& dateFormatter, std::vector<std::string>& argNames, std::string& argSeparator)
{
#define ADD_RULE_IF_NEEDED() \
    if (addNewRule) {        \
        rules.push_back({}); \
        addNewRule = false;  \
    }

    bool isParsingOk = true;
    bool addNewRule  = true;

    for (int argNbr = 1; argNbr < argc; ++argNbr) {
        const char* argStr = argv[argNbr];

        if (strcmp(argStr, "-h") == 0 || strcmp(argStr, "--help") == 0) {
            mode = Mode::Help;
        }

        else if (strcmp(argStr, "-lmin") == 0 && argNbr + 1 < argc) {
            const char* levelStr = argv[++argNbr];
            bool        found    = false;
            for (int l = 0; l < SSLOG_LEVEL_QTY; ++l) {
                if (strncmp(levelStr, LogSession::getLevelName(sslog::Level(l)), strlen(levelStr)) == 0) {
                    found = true;
                    ADD_RULE_IF_NEEDED();
                    rules.back().levelMin = sslog::Level(l);
                }
            }
            if (!found) {
                fprintf(stderr, "Error: '%s' is not a valid level name for -l\n", levelStr);
                isParsingOk = false;
            }
        } else if (strcmp(argStr, "-lmax") == 0 && argNbr + 1 < argc) {
            const char* levelStr = argv[++argNbr];
            bool        found    = false;
            for (int l = 0; l < SSLOG_LEVEL_QTY; ++l) {
                if (strncmp(levelStr, LogSession::getLevelName(sslog::Level(l)), strlen(levelStr)) == 0) {
                    found = true;
                    ADD_RULE_IF_NEEDED();
                    rules.back().levelMax = sslog::Level(l);
                }
            }
            if (!found) {
                fprintf(stderr, "Error: '%s' is not a valid level name for -lmax\n", levelStr);
                isParsingOk = false;
            }
        } else if (strcmp(argStr, "-bmin") == 0 && argNbr + 1 < argc) {
            ADD_RULE_IF_NEEDED();
            rules.back().bufferSizeMin = strtol(argv[++argNbr], nullptr, 10);
        } else if (strcmp(argStr, "-bmax") == 0 && argNbr + 1 < argc) {
            ADD_RULE_IF_NEEDED();
            rules.back().bufferSizeMax = strtol(argv[++argNbr], nullptr, 10);
        } else if (strcmp(argStr, "-c") == 0 && argNbr + 1 < argc) {
            ADD_RULE_IF_NEEDED();
            rules.back().category = argv[++argNbr];
        } else if (strcmp(argStr, "-nc") == 0 && argNbr + 1 < argc) {
            ADD_RULE_IF_NEEDED();
            rules.back().noCategory = argv[++argNbr];
        } else if (strcmp(argStr, "-t") == 0 && argNbr + 1 < argc) {
            ADD_RULE_IF_NEEDED();
            rules.back().thread = argv[++argNbr];
        } else if (strcmp(argStr, "-nt") == 0 && argNbr + 1 < argc) {
            ADD_RULE_IF_NEEDED();
            rules.back().noThread = argv[++argNbr];
        } else if (strcmp(argStr, "-f") == 0 && argNbr + 1 < argc) {
            ADD_RULE_IF_NEEDED();
            rules.back().format = argv[++argNbr];
        } else if (strcmp(argStr, "-nf") == 0 && argNbr + 1 < argc) {
            ADD_RULE_IF_NEEDED();
            rules.back().noFormat = argv[++argNbr];
        } else if (strcmp(argStr, "-a") == 0 && argNbr + 1 < argc) {
            ADD_RULE_IF_NEEDED();
            rules.back().arguments.push_back(argv[++argNbr]);
        }

        else if (strcmp(argStr, "-m") == 0) {
            mode = Mode::MonoText;
        } else if (strcmp(argStr, "-j") == 0) {
            mode = Mode::Json;
        } else if (strcmp(argStr, "-i") == 0) {
            mode = Mode::Infos;
        } else if (strcmp(argStr, "-v") == 0) {
            mode = Mode::Values;
            if (argNbr + 1 < argc && argv[argNbr + 1][0] != '-') {
                argNames.push_back(argv[++argNbr]);
            } else {
                argNames.push_back({});  // Empty string means timestamp
            }
        }

        else if (strcmp(argStr, "-u") == 0) {
            withUtcTime = true;
        } else if (strcmp(argStr, "-p") == 0 && argNbr + 1 < argc) {
            consoleFormatter = argv[++argNbr];
        } else if (strcmp(argStr, "-q") == 0 && argNbr + 1 < argc) {
            dateFormatter = argv[++argNbr];
        } else if (strcmp(argStr, "-s") == 0 && argNbr + 1 < argc) {
            argSeparator = argv[++argNbr];
        }

        else if (strcmp(argStr, "-o") == 0) {
            addNewRule = true;
        }

        else if (argStr[0] == '-') {
            fprintf(stderr, "Error: Command line option '%s' is unknown\n", argStr);
            isParsingOk = false;
        } else if (logDirPath.empty()) {
            logDirPath = argStr;
        } else {
            fprintf(stderr, "Error: Only one log directory path must be provided, not '%s' and '%s'\n", logDirPath.string().c_str(),
                    argStr);
            isParsingOk = false;
        }
    }

    if (logDirPath.empty()) { mode = Mode::Help; }
    return isParsingOk;
}

void
displayUsage(const char* programPath)
{
    printf(R"~(Usage:
     %s [options] <sslog dir>

<sslog dir>  : log directory

Definitions:
 <level>   possible values are trace, debug, info, warn, error, critical
 <pattern> is a search string, potentially containing wildcard. Ex: "*home*log*"

Options:
 -h           : this help

Filtering options:
 default       : no filtering
 -lmin <level> : filter-in  this level and above
 -lmax <level> : filter-out levels strictly above
 -bmin         : filter-in  logs with binary buffers at least this size
 -bmax         : filter-out logs with binary buffers strictly above this size
 -c <pattern>  : filter-in  categories that matches
 -nc <pattern> : filter-out categories that matches
 -t <pattern>  : filter-in  threads that matches
 -nt <pattern> : filter-out threads that matches
 -f <pattern>  : filter-in  format strings that matches. CAUTION: arguments are not considered
 -nf <pattern> : filter-out format strings that matches
 -a "<name><op><value>": filter-in argument based on the value given as a string.
                         <op> is between '=', '==', '>', '>=', '<', '<='
                         May be multiple occurences
                         Caution: equality and floats may have precision issues
                         Caution: no pattern matching on names and values
 -o            : end a serie of 'AND' filters and start a new one (OR)

Output modes:
 default      : colored text formatted logs
 -m           : monochrome text formatted logs
 -j           : JSON output
 -i           : information on the logs (date, stats, names, units, strings)
 -v <name>    : dump only the argument values per line. Empty name means 'timestamp'. Multiple options possible.

Output options:
 -u           : use UTC time instead of local time
 -p <format>  : formatter configuration of log display. Default "[%%L] [%%Y-%%m-%%dT%%H:%%M:%%S.%%f%%z] [%%c] [thread %%t] %%v%%Q"
 -q <format>  : formatter configuration of date (for output mode -j and -v). Default "%%G"
 -s <sep>     : separator string between argument values (for output mode -v). Default " "

Formatting catalog:
    %%t 	Thread id
    %%v 	The actual text to log
    %%c 	Category
    %%L 	The log level of the message
    %%l 	Short log level of the message
    %%a 	Abbreviated weekday name
    %%A 	Full weekday name
    %%b 	Abbreviated month name
    %%B 	Full month name
    %%y 	Year in 2 digits
    %%Y 	Year in 4 digits
    %%m 	Month
    %%d 	Day of month
    %%p 	AM/PM 	"AM"
    %%z 	ISO 8601 offset from UTC in timezone
    %%H 	Hours in 24 format
    %%h 	Hours in 12 format
    %%M 	Minutes
    %%S 	Seconds
    %%e 	Millisecond part of the current second
    %%f 	Microsecond part of the current second
    %%g 	Nanosecond  part of the current second
    %%E 	Milliseconds since epoch
    %%F 	Microseconds since epoch
    %%G 	Nanoseconds  since epoch
    %%I 	Milliseconds since start of the record
    %%J 	Microseconds since start of the record
    %%K 	Nanoseconds  since start of the record
    %%Q     end of line and multiline binary buffer dump
    %%q     ' (+ buffer of size N)' or nothing if empty
)~",
           programPath);
}

int
main(int argc, char** argv)
{
    // Parse input parameters
    Mode                     mode = Mode::ColorText;
    std::filesystem::path    logDirPath;
    std::vector<Rule>        rules;
    std::vector<std::string> argNames;
    std::string              consoleFormatter = "[%L] [%Y-%m-%dT%H:%M:%S.%f%z] [%c] [thread %t] %v%Q";
    std::string              dateFormatter    = "%G";
    std::string              argSeparator     = " ";
    bool                     withUtcTime      = false;

    if (!parseParameters(argc, argv, logDirPath, mode, withUtcTime, rules, consoleFormatter, dateFormatter, argNames, argSeparator)) {
        return 1;
    }

    if (mode == Mode::Help) {
        displayUsage(argv[0]);
        return 1;
    }

    // Initialise the log session
    std::string errorMessage;
    LogSession  session;
    if (!session.init(logDirPath, errorMessage)) {
        fprintf(stderr, "Error: %s\n", errorMessage.c_str());
        return 1;
    }

    // Display
    if (mode == Mode::Infos) {
        sslog::priv::TextFormatter fc;
        fc.init("%Y-%m-%dT%H:%M:%S.%g%z", withUtcTime, false, session.getUtcSystemClockOriginNs());
        char dateBuffer[128];
        fc.format(dateBuffer, sizeof(dateBuffer), session.getUtcSystemClockOriginNs(), SSLOG_LEVEL_QTY, "", "", "", nullptr, 0, false);

        printf("Start date   : %s (%s)\n", dateBuffer, withUtcTime ? "UTC" : "local time");
        printf("Duration     : %.3f s\n", 1e-9 * session.getLogDurationNs());
        printf("Size         : %.3f MB\n", 1e-6 * session.getLogByteQty());
        printf("Quantity     : %lu logs\n", session.getLogQty());
        printf("Arg quantity : %lu (%.1f B/log) (%.1f B/arg)\n", session.getArgQty(),
               (double)session.getLogByteQty() / (double)session.getLogQty(),
               (double)session.getLogByteQty() / (double)session.getArgQty());
        printf("Clock res.   : %f ns\n", session.getClockResolutionNs());
        printf("Arg names    : ");
        bool isFirst = true;
        for (const std::string& s : session.getArgNameStrings()) {
            if (!isFirst) { printf(", "); }
            isFirst = false;
            printf("%s", s.c_str());
        }
        printf("\nArg units    : ");
        isFirst = true;
        for (const std::string& s : session.getArgUnitStrings()) {
            if (!isFirst) { printf(", "); }
            isFirst = false;
            printf("%s", s.c_str());
        }
        printf("\nStrings      : %" PRId64 "\n", session.getIndexedStringQty());
        for (uint32_t i = 0; i < session.getIndexedStringQty(); ++i) {
            uint8_t flag = session.getIndexedStringFlags(i);
            printf("               %-3u [%s%s%s%s]  %s\n", i, (flag & FlagCategory) ? "C" : " ", (flag & FlagThread) ? "T" : " ",
                   (flag & FlagFormat) ? "F" : " ", (flag & FlagArgValue) ? "A" : " ", session.getIndexedString(i));
        }
    }

    else if (mode == Mode::Json) {
        LogHandlerJson handler(session, dateFormatter, withUtcTime);
        if (!session.query(rules, [&handler](const LogStruct& log) { handler.notifyLog(log); }, errorMessage)) {
            fprintf(stderr, "Error: %s\n", errorMessage.c_str());
            return 1;
        }
    }

    else if (mode == Mode::Values) {
        LogHandlerValues handler(session, dateFormatter, withUtcTime, argNames, argSeparator);
        if (!session.query(rules, [&handler](const LogStruct& log) { handler.notifyLog(log); }, errorMessage)) {
            fprintf(stderr, "Error: %s\n", errorMessage.c_str());
            return 1;
        }
    }

    else {
        LogHandlerText handler(session, consoleFormatter, withUtcTime, (mode == Mode::ColorText));
        if (!session.query(rules, [&handler](const LogStruct& log) { handler.notifyLog(log); }, errorMessage)) {
            fprintf(stderr, "Error: %s\n", errorMessage.c_str());
            return 1;
        }
    }

    return 0;
}
