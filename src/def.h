#pragma once
#include <unordered_map>
#include <string>
#include <filesystem>

#if defined(_DEBUG)
#define VERIFY(exp, ret)                                                                                               \
    if (!(exp))                                                                                                        \
    {                                                                                                                  \
        DebugBreak();                                                                                                  \
        throw exception(GetErrorInfo(ret));                                                                            \
    }                                                                                                                  \
    void(0)

#else
#define VERIFY(exp, ret)                                                                                               \
    if (!(exp))                                                                                                        \
        throw exception(GetErrorInfo(ret));
#endif

using FileList = std::unordered_map<std::string, std::filesystem::path>;

struct Params
{
    bool file_list = false;
};

enum class Error
{
    Ok,
    ArgError,
    InputNotFolder,
    InputReadFailed,
    InputDocumentError,
    OutputIsFolder,
    OutputCantWrite,
    ArchiveCreateFailed,
    ArchiveFileCreateFailed,
    ReadFileFailed,
    ArchiveFileWriteFailed,
    ArchiveFileFinishFailed,
    ArchiveFlushFailed,
    ArchiveCloseFailed,
};

constexpr char* PARAM_FILELIST = "filelist";
constexpr char* PARAM_OUTPUT = "output";
