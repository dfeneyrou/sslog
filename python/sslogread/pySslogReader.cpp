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

// This file is a Python implementation of the sslogread library which manipulates
// log folder created with  `sslog`

// System includes
#include <algorithm>
#include <vector>

// CPython includes
#include "Python.h"
#include "sslogread/sslogread.h"
#include "sslogread/utils.h"

// Define the Session capsule
// ==========================

static sslogread::LogSession*
getSessionFromPyCapsule(PyObject* obj)
{
    return (sslogread::LogSession*)PyCapsule_GetPointer(obj, "sslogread.LogSession");
}

static void
deleteSession(PyObject* obj)
{
    delete getSessionFromPyCapsule(obj);
}

static PyObject*
getPyCapsuleFromSession(sslogread::LogSession* session, int mustFree)
{
    return PyCapsule_New(session, "sslogread.LogSession", mustFree ? deleteSession : nullptr);
}

bool
getLevel(const char* levelStr, sslog::Level& level)
{
    for (int l = 0; l < SSLOG_LEVEL_QTY; ++l) {
        if (strcmp(levelStr, sslogread::LogSession::getLevelName(sslog::Level(l))) == 0) {
            level = sslog::Level(l);
            return true;
        }
    }
    return false;
}

// Define the NameTuple Log structure
// ==================================

static PyStructSequence_Field LogFields[] = {{"level", "Level"},          {"timestampUtcNs", "Timestamp in nanosecond since epoch"},
                                             {"category", "Category"},    {"thread", "Thread name"},
                                             {"format", "Format string"}, {"arguments", "List of arguments"},
                                             {"buffer", "Binary buffer"}, {NULL}};

static PyStructSequence_Desc LogDesc = {"sslogread.Log", NULL, LogFields, 7};

PyTypeObject* LogTupleType = nullptr;

// Define a Session class
// =======================

// Structure to hold the Session object's data
typedef struct {
    PyObject_HEAD PyObject* sessionObj;
} SessionObject;

static PyObject*
session_query(SessionObject* self, PyObject* args)
{
#define READ_DICT_STRING_ITEM(keyName, outputName)                                        \
    {                                                                                     \
        PyObject* pyObj = PyDict_GetItemString(dictItem, keyName);                        \
        if (pyObj != nullptr) {                                                           \
            if (!PyUnicode_Check(pyObj)) {                                                \
                PyErr_SetString(PyExc_TypeError, "'" keyName "' value must be a string"); \
                return 0;                                                                 \
            } else {                                                                      \
                outputName = PyUnicode_AsUTF8(pyObj);                                     \
            }                                                                             \
        }                                                                                 \
    }
#define READ_DICT_UINT32_ITEM(keyName, outputName)                                         \
    {                                                                                      \
        PyObject* pyObj = PyDict_GetItemString(dictItem, keyName);                         \
        if (pyObj != nullptr) {                                                            \
            if (!PyLong_Check(pyObj)) {                                                    \
                PyErr_SetString(PyExc_TypeError, "'" keyName "' value must be a boolean"); \
                return 0;                                                                  \
            } else {                                                                       \
                outputName = PyLong_AsUnsignedLong(pyObj);                                 \
            }                                                                              \
        }                                                                                  \
    }

    // Get the C++ session objects
    if (!self->sessionObj) {
        PyErr_SetString(PyExc_AttributeError, "session field is not set");
        return NULL;
    }
    sslogread::LogSession* session = getSessionFromPyCapsule(self->sessionObj);
    if (!session) {
        PyErr_SetString(PyExc_AttributeError, "unable to retrieve the internal session object");
        return NULL;
    }

    std::vector<sslogread::Rule> rules;

    // Loop on input rules
    Py_ssize_t ruleQty = PyTuple_Size(args);
    for (int ruleNbr = 0; ruleNbr < ruleQty; ruleNbr++) {
        PyObject* dictItem = PyTuple_GetItem(args, ruleNbr);
        if (!PyDict_Check(dictItem)) {
            PyErr_SetString(PyExc_TypeError, "Arguments must be dictionaries");
            return nullptr;
        }
        const char*              levelMinStr   = "trace";
        const char*              levelMaxStr   = "off";
        const char*              category      = "";
        const char*              thread        = "";
        const char*              format        = "";
        uint32_t                 minBufferSize = 0;
        uint32_t                 maxBufferSize = 0xFFFFFFFF;
        const char*              noCategory    = "";
        const char*              noThread      = "";
        const char*              noFormat      = "";
        std::vector<std::string> arguments;

        // Check for unknown parameter
        PyObject * pyKey, *pyValue;
        Py_ssize_t pos = 0;
        while (PyDict_Next(dictItem, &pos, &pyKey, &pyValue)) {
            if (!PyUnicode_Check(pyKey)) {
                PyErr_SetString(PyExc_TypeError, "Dictionary keys must be strings");
                return nullptr;
            }
            const char* cKey = PyUnicode_AsUTF8(pyKey);
            if (strcmp(cKey, "level_min") != 0 && strcmp(cKey, "level_max") != 0 && strcmp(cKey, "category") != 0 &&
                strcmp(cKey, "thread") != 0 && strcmp(cKey, "format") != 0 && strcmp(cKey, "buffer_size_min") != 0 &&
                strcmp(cKey, "buffer_size_max") != 0 && strcmp(cKey, "no_category") != 0 && strcmp(cKey, "no_thread") != 0 &&
                strcmp(cKey, "no_format") != 0 && strcmp(cKey, "arguments") != 0) {
                char tmpStr[256];
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)  // Disable Windows's secure API warnings on snprintf
#endif
                snprintf(tmpStr, sizeof(tmpStr),
                         "Unknown dictionary key '%s' for log filter.\nIt shall be among: level_min, level_max, category, thread, format, "
                         "buffer_size_min, buffer_size_max, no_category, no_thread, no_format, arguments",
                         cKey);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
                PyErr_SetString(PyExc_TypeError, tmpStr);
                return nullptr;
            }
        }

        // Read the present parameters
        READ_DICT_STRING_ITEM("level_min", levelMinStr);
        READ_DICT_STRING_ITEM("level_max", levelMaxStr);
        READ_DICT_STRING_ITEM("category", category);
        READ_DICT_STRING_ITEM("thread", thread);
        READ_DICT_STRING_ITEM("format", format);
        READ_DICT_UINT32_ITEM("buffer_size_min", minBufferSize);
        READ_DICT_UINT32_ITEM("buffer_size_max", maxBufferSize);
        READ_DICT_STRING_ITEM("no_category", noCategory);
        READ_DICT_STRING_ITEM("no_thread", noThread);
        READ_DICT_STRING_ITEM("no_format", noFormat);

        sslog::Level levelMin, levelMax;
        if (!getLevel(levelMinStr, levelMin)) {
            PyErr_SetString(PyExc_TypeError, "'level_min' must be among 'trace', 'debug', 'info', 'warn', 'error', 'critical', 'off'");
            return 0;
        }
        if (!getLevel(levelMaxStr, levelMax)) {
            PyErr_SetString(PyExc_TypeError, "'level_max' must be among 'trace', 'debug', 'info', 'warn', 'error', 'critical', 'off'");
            return 0;
        }

        PyObject* pyArgument = PyDict_GetItemString(dictItem, "arguments");
        if (pyArgument != nullptr) {
            if (!PyList_Check(pyArgument)) {
                PyErr_SetString(PyExc_TypeError, "'arguments' must be a list of strings");
                return 0;
            }
            int argQty = (int)(pyArgument ? PyList_Size(pyArgument) : 0);
            for (int argNbr = 0; argNbr < argQty; ++argNbr) {
                PyObject* stringItem = PyList_GetItem(pyArgument, argNbr);
                if (!PyUnicode_Check(stringItem)) {
                    PyErr_SetString(PyExc_TypeError, "Content of 'arguments' must be strings");
                    return 0;
                }
                arguments.push_back(PyUnicode_AsUTF8(stringItem));
            }
        }

        rules.push_back(
            {levelMin, levelMax, category, thread, format, arguments, minBufferSize, maxBufferSize, noCategory, noThread, noFormat});
    }

    char              filledFormatBuffer[8192];
    std::vector<char> base64Output;
    std::string       errorMessage;
    PyObject*         outputList = PyList_New(0);

    // Parse the log and get the raw output
    bool queryStatus = session->query(
        rules,
        [outputList, session, &base64Output, &filledFormatBuffer](int /*ruleIdx*/, const sslogread::LogStruct& log) {
            PyObject* pyLog = PyStructSequence_New(LogTupleType);
            PyStructSequence_SetItem(pyLog, 0, PyUnicode_FromString(sslogread::LogSession::getLevelName(log.level)));
            PyStructSequence_SetItem(pyLog, 1, PyLong_FromLongLong(log.timestampUtcNs));
            PyStructSequence_SetItem(pyLog, 2, PyUnicode_FromString(session->getIndexedString(log.categoryIdx)));
            PyStructSequence_SetItem(pyLog, 3, PyUnicode_FromString(session->getIndexedString(log.threadIdx)));

            sslogread::vsnprintfLog((char*)filledFormatBuffer, sizeof(filledFormatBuffer), session->getIndexedString(log.formatIdx),
                                    log.args, session);
            PyStructSequence_SetItem(pyLog, 4, PyUnicode_FromString(filledFormatBuffer));

            PyObject*                                     argList         = PyList_New(log.args.size());
            const std::vector<sslogread::ArgNameAndUnit>& argsNameAndUnit = session->getIndexedStringArgNameAndUnit(log.formatIdx);

            // Format arguments
            for (uint32_t argIdx = 0; argIdx < log.args.size(); ++argIdx) {
                PyObject*   argTuple = PyTuple_New(3);
                const char* argName  = (argIdx < argsNameAndUnit.size()) ? argsNameAndUnit[argIdx].name.c_str() : "";
                const char* argUnit  = (argIdx < argsNameAndUnit.size()) ? argsNameAndUnit[argIdx].unit.c_str() : "";
                PyTuple_SetItem(argTuple, 0, PyUnicode_FromString(argName));
                PyTuple_SetItem(argTuple, 1, PyUnicode_FromString(argUnit));
                const sslogread::Arg& arg = log.args[argIdx];
                switch (arg.pType) {
                    case sslogread::ArgType::S32:
                        PyTuple_SetItem(argTuple, 2, PyLong_FromLong(arg.vS32));
                        break;
                    case sslogread::ArgType::U32:
                        PyTuple_SetItem(argTuple, 2, PyLong_FromUnsignedLong(arg.vU32));
                        break;
                    case sslogread::ArgType::S64:
                        PyTuple_SetItem(argTuple, 2, PyLong_FromLongLong(arg.vS64));
                        break;
                    case sslogread::ArgType::U64:
                        PyTuple_SetItem(argTuple, 2, PyLong_FromUnsignedLongLong(arg.vU64));
                        break;
                    case sslogread::ArgType::Float:
                        PyTuple_SetItem(argTuple, 2, PyFloat_FromDouble(arg.vFloat));
                        break;
                    case sslogread::ArgType::Double:
                        PyTuple_SetItem(argTuple, 2, PyFloat_FromDouble(arg.vDouble));
                        break;
                    case sslogread::ArgType::StringIdx:
                        PyTuple_SetItem(argTuple, 2, PyUnicode_FromString(session->getIndexedString(arg.vStringIdx)));
                };
                PyList_SetItem(argList, argIdx, argTuple);
            }
            PyStructSequence_SetItem(pyLog, 5, argList);

            // Buffer
            if (log.buffer.empty()) {
                Py_INCREF(Py_None);
                PyStructSequence_SetItem(pyLog, 6, Py_None);
            } else {
                sslogread::base64Encode(log.buffer, base64Output);
                PyStructSequence_SetItem(pyLog, 6, PyUnicode_FromString(base64Output.data()));
            }

            PyList_Append(outputList, pyLog);
            Py_DECREF(pyLog);  // 'Append' takes a reference
            return true;
        },
        errorMessage);

    if (!queryStatus) {
        PyErr_SetString(PyExc_TypeError, errorMessage.c_str());
        return 0;
    }

    return outputList;
}

static PyObject*
session_getStrings(SessionObject* self, PyObject* args, PyObject* kwargs)
{
    // Get the C++ session objects
    if (!self->sessionObj) {
        PyErr_SetString(PyExc_AttributeError, "session field is not set");
        return NULL;
    }
    sslogread::LogSession* session = getSessionFromPyCapsule(self->sessionObj);
    if (!session) {
        PyErr_SetString(PyExc_AttributeError, "unable to retrieve the internal session object");
        return NULL;
    }

    // Parse parameters
    static char const* kwlist[] = {"pattern", "in_category", "in_thread", "in_format", "in_arg_name", "in_arg_value", "in_arg_unit", NULL};
    const char*        pattern  = "";
    int                in_category  = 0;
    int                in_thread    = 0;
    int                in_format    = 0;
    int                in_arg_name  = 0;
    int                in_arg_value = 0;
    int                in_arg_unit  = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|spppppp", (char**)kwlist, &pattern, &in_category, &in_thread, &in_format, &in_arg_name,
                                     &in_arg_value, &in_arg_unit)) {
        return 0;
    }

    // Compute the flag mask
    uint8_t mask = ((in_category ? sslogread::FlagCategory : 0) | (in_thread ? sslogread::FlagThread : 0) |
                    (in_format ? sslogread::FlagFormat : 0) | (in_arg_value ? sslogread::FlagArgValue : 0));
    if (mask + in_arg_name + in_arg_unit == 0) mask = 0xFF;  // Default is all strings

    PyObject* outputList = PyList_New(0);
    for (uint32_t stringIdx : session->getStringIndexes(pattern, mask)) {
        PyObject* s = PyUnicode_FromString(session->getIndexedString(stringIdx));
        if (s) {
            PyList_Append(outputList, s);
            Py_DECREF(s);  // 'Append' increments the reference count
        }
    }

    std::vector<sslogread::PatternChunk> patternChunks;
    if (in_arg_name || in_arg_unit) { patternChunks = sslogread::getPatternChunks(pattern); }

    if (in_arg_name) {
        for (const std::string& name : session->getArgNameStrings()) {
            if (sslogread::isStringMatching(patternChunks, name.c_str())) {
                PyObject* s = PyUnicode_FromString(name.c_str());
                if (s) {
                    PyList_Append(outputList, s);
                    Py_DECREF(s);
                }
            }
        }
    }

    if (in_arg_unit) {
        for (const std::string& unit : session->getArgUnitStrings()) {
            if (sslogread::isStringMatching(patternChunks, unit.c_str())) {
                PyObject* s = PyUnicode_FromString(unit.c_str());
                if (s) {
                    PyList_Append(outputList, s);
                    Py_DECREF(s);
                }
            }
        }
    }

    // Return the filtered list of strings
    return outputList;
}

// Method Table for Session objects
static PyMethodDef Session_methods[] = {
    {"query", (PyCFunction)session_query, METH_VARARGS, "Query the logs and return structured data"},
    {"get_strings", (PyCFunction)session_getStrings, METH_VARARGS | METH_KEYWORDS, "Get the strings per type of use"},
    {NULL}};

static PyObject*
Session_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    SessionObject* self;
    self = (SessionObject*)type->tp_alloc(type, 0);
    if (self != NULL) { self->sessionObj = NULL; }
    return (PyObject*)self;
}

static void
Session_dealloc(SessionObject* self)
{
    Py_XDECREF(self->sessionObj);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static int
Session_init(SessionObject* self, PyObject* args, PyObject* kwds)
{
    static const char* kwlist[] = {"sessionObj", NULL};

    PyObject* sessionObj = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", (char**)kwlist, &sessionObj)) return -1;

    if (sessionObj) {
        Py_INCREF(sessionObj);
        self->sessionObj = sessionObj;
    }

    return 0;
}

// Type Object for Session
#if 0
// clang-format off
PyTypeObject SessionType = {
    .ob_base      = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "sslogread.LogSession",
    .tp_basicsize = sizeof(SessionObject),
    .tp_itemsize  = 0,
    .tp_dealloc   = (destructor)Session_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc       = PyDoc_STR("sslogread.LogSession object. Reads and manipulates sslog data."),
    .tp_methods   = Session_methods,
    .tp_init      = (initproc)Session_init,
    .tp_new       = Session_new,
};
// clang-format on
#else
// Initialization is in 2 parts, due to Windows's refusal of designated initializers in C++17
PyTypeObject SessionType = {PyVarObject_HEAD_INIT(NULL, 0)};

#endif

// Public API
// ==========

static PyObject*
load(PyObject* Py_UNUSED(self), PyObject* args)
{
    // Get the parameters
    char* logDirPath = nullptr;
    if (!PyArg_ParseTuple(args, "s", &logDirPath)) {
        PyErr_SetString(PyExc_TypeError, "Unable to decode the path parameter. A string is expected.");
        return 0;
    }

    // Read the base information
    std::string            errorMessage;
    sslogread::LogSession* session = new sslogread::LogSession();
    if (!session->init(logDirPath, errorMessage)) {
        PyErr_SetString(PyExc_RuntimeError, errorMessage.c_str());
        delete session;
        return 0;
    }

    // Encapsulate the C++ object and create the Python sslogread.LogSession class
    PyObject* sessionObj = getPyCapsuleFromSession(session, 1);
    PyObject* pySession  = PyObject_CallFunction((PyObject*)&SessionType, "O", sessionObj);

    return pySession;
}

// Python module initialization
// ============================

static PyMethodDef moduleMethods[] = {
    {"load", load, METH_VARARGS, "Loads information from the log directory"}, {0, 0, 0, 0}  // End of list of functions
};

PyMODINIT_FUNC
PyInit_sslogread(void)
{
    static struct PyModuleDef sslogreadModuleDef = {
        PyModuleDef_HEAD_INIT, "sslogread", "Python sslog reader", -1, moduleMethods, 0, 0, 0, 0};

    PyObject* m = PyModule_Create(&sslogreadModuleDef);

    // Create the NameTuple
    LogTupleType = PyStructSequence_NewType(&LogDesc);
    if (LogTupleType == nullptr) { return 0; }

    // Create the Session class
    SessionType.tp_name      = "sslogread.LogSession";
    SessionType.tp_basicsize = sizeof(SessionObject);
    SessionType.tp_itemsize  = 0;
    SessionType.tp_dealloc   = (destructor)Session_dealloc;
    SessionType.tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    SessionType.tp_doc       = PyDoc_STR("sslogread.LogSession object. Reads and manipulates sslog data.");
    SessionType.tp_methods   = Session_methods;
    SessionType.tp_init      = (initproc)Session_init;
    SessionType.tp_new       = Session_new;
    if (PyType_Ready(&SessionType) < 0) { return 0; }
    Py_INCREF(&SessionType);
    if (PyModule_AddObject(m, "LogSession", (PyObject*)&SessionType) < 0) {
        Py_DECREF(&SessionType);
        return 0;
    }

    return m;
}
