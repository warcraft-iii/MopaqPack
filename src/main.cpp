#include "cmdline.h"
#include <StormLib.h>
#include <filesystem>
#include <fstream>

#define VERIFY(exp, ret)                                                                                               \
    if (!(exp))                                                                                                        \
    {                                                                                                                  \
        return (ret);                                                                                                  \
    }                                                                                                                  \
    void(0)

using namespace std;

string w2s(const wstring& input)
{
    char buf[2048] = {0};
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), (int)input.length(), buf, ARRAYSIZE(buf), nullptr, nullptr);
    return buf;
}

enum class Error
{
    Ok,
    ArgError,
    InputNotFolder,
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

string getErrorInfo(Error err)
{
    switch (err)
    {
    case Error::Ok:
        break;
    case Error::OutputIsFolder:
        return "Output can not be a folder";
    case Error::OutputCantWrite:
        return "Output write failed";
    case Error::ArchiveCreateFailed:
        return "Create archive failed";
    case Error::ArchiveFileCreateFailed:
        return "Create archive file failed";
    case Error::ReadFileFailed:
        return "Read file failed";
    case Error::ArchiveFileWriteFailed:
        return "Write archive file failed";
    case Error::ArchiveFileFinishFailed:
        return "Finish archive file failed";
    case Error::ArchiveFlushFailed:
        return "Flush archive failed";
    case Error::ArchiveCloseFailed:
        return "Close archive failed";
    case Error::ArgError:
        return "Argument error";
    case Error::InputNotFolder:
        return "Input need a folder";
    default:
        return "";
    }
}

Error run(const string& input, const string& output)
{
    VERIFY(!filesystem::is_directory(output), Error::OutputIsFolder);
    VERIFY(!filesystem::exists(output) || filesystem::remove(output), Error::OutputCantWrite);
    VERIFY(filesystem::is_directory(input), Error::InputNotFolder);

    list<filesystem::path> files;
    for (const auto& p : filesystem::recursive_directory_iterator(input))
    {
        files.push_back(p.path());
    }

    HANDLE archive;
    VERIFY(SFileCreateArchive(output.c_str(), 0, files.size(), &archive), Error::ArchiveCreateFailed);

    for (auto& file : files)
    {
        HANDLE fileHandle;
        auto name = relative(file, input);
        VERIFY(SFileCreateFile(archive, w2s(name).c_str(), 0, file_size(file), 0, MPQ_FILE_COMPRESS, &fileHandle),
               Error::ArchiveFileCreateFailed);

        ifstream f;
        f.open(file.c_str(), ios::in | ios::binary);
        VERIFY(f.is_open(), Error::ReadFileFailed);

        char buffer[BUFSIZ] = {0};
        while (!f.eof())
        {
            f.read(buffer, BUFSIZ);
            auto size = f.gcount();
            VERIFY(SFileWriteFile(fileHandle, buffer, size, MPQ_COMPRESSION_ZLIB), Error::ArchiveFileWriteFailed);
        }
        VERIFY(SFileFinishFile(fileHandle), Error::ArchiveFileFinishFailed);
    }

    VERIFY(SFileFlushArchive(archive), Error::ArchiveFlushFailed);
    VERIFY(SFileCloseArchive(archive), Error::ArchiveCloseFailed);

    return Error::Ok;
}

int main(int argc, char** argv)
{
    cmdline::parser parser;
    parser.add<string>("output", 'o', "Output path", true);
    if (!parser.parse(argc, argv))
    {
        cout << parser.usage() << endl;
        return -1;
    }

    if (parser.rest().empty())
    {
        cout << parser.usage() << endl;
        return -1;
    }

    auto input = parser.rest()[0];
    auto output = parser.get<string>("output");

    auto err = run(input, output);
    switch (err)
    {
    case Error::OutputIsFolder:
        break;
    default:
        cout << getErrorInfo(err) << endl;
        break;
    }
    return (int)err;
}