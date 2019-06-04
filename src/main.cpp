#include "cmdline.h"
#include <StormLib.h>
#include <filesystem>
#include <fstream>
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <cstdio>
#include "def.h"

using namespace std;

const char* GetErrorInfo(Error err)
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

void Run(const FileList& input, const string& output, const Params& params)
{
    VERIFY(!filesystem::is_directory(output), Error::OutputIsFolder);
    VERIFY(!filesystem::exists(output) || filesystem::remove(output), Error::OutputCantWrite);

    HANDLE archive;
    SFILE_CREATE_MPQ ci;
    DWORD flags = 0;
    memset(&ci, 0, sizeof(SFILE_CREATE_MPQ));
    ci.cbSize = sizeof(SFILE_CREATE_MPQ);
    ci.dwMpqVersion = (flags & MPQ_CREATE_ARCHIVE_VMASK) >> FLAGS_TO_FORMAT_SHIFT;
    ci.dwStreamFlags = STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE;
    ci.dwFileFlags1 = flags & MPQ_CREATE_LISTFILE ? MPQ_FILE_DEFAULT_INTERNAL : 0;
    ci.dwFileFlags2 = flags & MPQ_CREATE_ATTRIBUTES ? MPQ_FILE_DEFAULT_INTERNAL : 0;
    ci.dwFileFlags3 = flags & MPQ_CREATE_SIGNATURE ? MPQ_FILE_DEFAULT_INTERNAL : 0;
    ci.dwAttrFlags =
        flags & MPQ_CREATE_ATTRIBUTES ? MPQ_ATTRIBUTE_CRC32 | MPQ_ATTRIBUTE_FILETIME | MPQ_ATTRIBUTE_MD5 : 0;
    ci.dwSectorSize = ci.dwMpqVersion >= MPQ_FORMAT_VERSION_3 ? 0x4000 : 0x1000;
    ci.dwRawChunkSize = ci.dwMpqVersion >= MPQ_FORMAT_VERSION_4 ? 0x4000 : 0;
    ci.dwMaxFileCount = input.size();

    if (ci.dwMpqVersion >= MPQ_FORMAT_VERSION_3 && flags & MPQ_CREATE_ATTRIBUTES)
    {
        ci.dwAttrFlags |= MPQ_ATTRIBUTE_PATCH_BIT;
    }

    if (params.file_list)
    {
        ci.dwFileFlags1 = MPQ_FILE_DEFAULT_INTERNAL;
    }

    VERIFY(SFileCreateArchive2(output.c_str(), &ci, &archive), Error::ArchiveCreateFailed);

    for (auto& iter : input)
    {
        auto name = iter.first;
        auto file = iter.second;

        HANDLE fileHandle;

        VERIFY(
            SFileCreateFile(archive, name.c_str(), 0, filesystem::file_size(file), 0, MPQ_FILE_COMPRESS, &fileHandle),
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
}

void GenerateFileList(const string& input, FileList& files)
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
    parser.add<string>(PARAM_OUTPUT, 'o', "Output path", true);
    parser.add<bool>(PARAM_FILELIST, 'f', "Generate file list", false, false);

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
    auto output = parser.get<string>(PARAM_OUTPUT);

    try
    {
        Params params = {parser.get<bool>(PARAM_FILELIST)};
        FileList fileList;
        GenerateFileList(input, fileList);
        Run(fileList, output, params);
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
    }
    return 0;
}
