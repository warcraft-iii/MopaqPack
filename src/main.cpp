#include "cmdline.h"
#include <StormLib.h>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <cstdio>

#if defined(_DEBUG)

#define VERIFY(exp, ret)                                                                                               \
    if (!(exp))                                                                                                        \
    {                                                                                                                  \
        DebugBreak();                                                                                                  \
        return (ret);                                                                                                  \
    }                                                                                                                  \
    void(0)

#else

#define VERIFY(exp, ret)                                                                                               \
    if (!(exp))                                                                                                        \
    {                                                                                                                  \
        return (ret);                                                                                                  \
    }                                                                                                                  \
    void(0)

#endif

using namespace std;

using FileList = unordered_map<string, filesystem::path>;

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
        break;
    }
    return "";
}

Error run(const FileList& input, const string& output)
{
    VERIFY(!filesystem::is_directory(output), Error::OutputIsFolder);
    VERIFY(!filesystem::exists(output) || filesystem::remove(output), Error::OutputCantWrite);
    // VERIFY(filesystem::is_directory(input), Error::InputNotFolder);

    // list<filesystem::path> files;
    // for (const auto& p : filesystem::recursive_directory_iterator(input))
    // {
    //     files.push_back(p.path());
    // }

    HANDLE archive;
    VERIFY(SFileCreateArchive(output.c_str(), 0, input.size(), &archive), Error::ArchiveCreateFailed);

    for (auto& iter : input)
    {
        auto name = iter.first;
        auto file = iter.second;

        HANDLE fileHandle;

        VERIFY(SFileCreateFile(archive, name.c_str(), 0, file_size(file), 0, MPQ_FILE_COMPRESS, &fileHandle),
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

Error generateFileList(const string& input, FileList& files)
{
    if (filesystem::is_directory(input))
    {
        for (const auto& p : filesystem::recursive_directory_iterator(input))
        {
            if (is_directory(p.path()))
            {
                continue;
            }
            auto name = relative(p.path(), input);
            files[name.u8string()] = p.path();
        }
    }
    else
    {
        constexpr auto length = 2048;
        char buffer[length] = {0};
        auto fp = fopen(input.c_str(), "r");
        VERIFY(fp, Error::InputReadFailed);

        rapidjson::FileReadStream stream(fp, buffer, length);
        rapidjson::Document doc;
        doc.ParseStream(stream);
        fclose(fp);
        VERIFY(doc.IsArray(), Error::InputDocumentError);

        for (auto item = doc.Begin(); item != doc.End(); ++item)
        {
            VERIFY(item->IsArray(), Error::InputDocumentError);
            VERIFY(item->Size() == 2, Error::InputDocumentError);

            auto& name = item->GetArray()[0];
            auto& file = item->GetArray()[1];

            VERIFY(name.IsString(), Error::InputDocumentError);
            VERIFY(file.IsString(), Error::InputDocumentError);

            files[name.GetString()] = filesystem::path(file.GetString());
        }
    }
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

    FileList fileList;
    auto err = generateFileList(input, fileList);
    if (err != Error::Ok)
    {
        cout << getErrorInfo(err) << endl;
    }

    err = run(fileList, output);
    if (err != Error::Ok)
    {
        cout << getErrorInfo(err) << endl;
    }
    return static_cast<int>(err);
}
