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
        throw exception(get_error_info(ret));                                                                          \
    }                                                                                                                  \
    void(0)

#else

#define VERIFY(exp, ret)                                                                                               \
    if (!(exp))                                                                                                        \
        throw exception(get_error_info(ret));

#endif

using namespace std;

using FileList = unordered_map<string, filesystem::path>;

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

const char* get_error_info(Error err)
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
    case Error::InputReadFailed:
        return "Input read failed";
    case Error::InputDocumentError:
        return "Input document error";
    default:
        break;
    }
    return "";
}

void run(const FileList& input, const string& output)
{
    VERIFY(!filesystem::is_directory(output), Error::OutputIsFolder);
    VERIFY(!filesystem::exists(output) || filesystem::remove(output), Error::OutputCantWrite);

    HANDLE archive;
    VERIFY(SFileCreateArchive(output.c_str(), 0, input.size(), &archive), Error::ArchiveCreateFailed);

    for (auto& iter : input)
    {
        auto name = iter.first;
        auto file = iter.second;

        HANDLE file_handle;

        VERIFY(SFileCreateFile(archive, name.c_str(), 0, file_size(file), 0, MPQ_FILE_COMPRESS, &file_handle),
               Error::ArchiveFileCreateFailed);

        ifstream f;
        f.open(file.c_str(), ios::in | ios::binary);
        VERIFY(f.is_open(), Error::ReadFileFailed);

        char buffer[BUFSIZ] = {0};
        while (!f.eof())
        {
            f.read(buffer, BUFSIZ);
            auto size = f.gcount();
            VERIFY(SFileWriteFile(file_handle, buffer, size, MPQ_COMPRESSION_ZLIB), Error::ArchiveFileWriteFailed);
        }
        VERIFY(SFileFinishFile(file_handle), Error::ArchiveFileFinishFailed);
    }

    VERIFY(SFileFlushArchive(archive), Error::ArchiveFlushFailed);
    VERIFY(SFileCloseArchive(archive), Error::ArchiveCloseFailed);
}

void generate_file_list(const string& input, FileList& files)
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

    try
    {
        FileList file_list;
        generate_file_list(input, file_list);
        run(file_list, output);
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
    }
    return 0;
}
