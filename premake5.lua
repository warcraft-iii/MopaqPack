-- premake5.lua
-- @Author : Dencer (tdaddon@163.com)
-- @Link   : https://dengsir.github.io
-- @Date   : 7/15/2019, 3:45:52 AM

workspace 'MopaqPack'
    configurations { 'Debug', 'Release' }
    location '.build'
    toolset 'v142'
    symbols 'Full'
    architecture 'x86'
    characterset 'MBCS'
    staticruntime 'On'
    dependson { 'ALL_BUILD' }

    startproject 'MopaqPack'

    flags {
        'MultiProcessorCompile',
        'Maps',
    }

    defines { 'WIN32', '_WINDOWS' }

    filter 'configurations:Debug'
        defines { '_DEBUG' }
    filter 'configurations:Release'
        defines { 'NDEBUG' }
        optimize 'On'

project 'ALL_BUILD'
    kind 'Utility'

    files {
        'premake5.lua',
    }

    filter 'files:premake5.lua'
        buildmessage 'Premaking'
        buildinputs { 'init.bat' }
        buildcommands { '../init.bat' }
        buildoutputs { '.build/ALL_BUILD.vcxproj' }

project 'MopaqPack'
    kind 'ConsoleApp'
    language 'C++'
    cppdialect 'C++17'
    links { 'StormLib' }

    files {
        'src/*.cpp',
        'src/*.h',
    }

    includedirs {
        '3rd/StormLib/src',
        '3rd/rapidjson/include',
        '3rd/cmdline'
    }

group '3rd'
    project 'StormLib'
        kind 'StaticLib'
        language 'C++'

    files {
        '3rd/StormLib/src/**.c',
        '3rd/StormLib/src/**.cpp',
        '3rd/StormLib/src/**.h',
    }

    removefiles {
        '3rd/StormLib/src/adpcm/*_old.*',
        '3rd/StormLib/src/pklib/crc32.c',
        '3rd/StormLib/src/zlib/compress.c'
    }

    vpaths {
        ['Source Files/**'] = {
            '3rd/StormLib/src/**.c',
            '3rd/StormLib/src/**.cpp',
        },
        ['Header Files/**'] = {
            '3rd/StormLib/src/**.h',
        }
    }

    defines { '_LIB' }

    filter 'configurations:Debug'
        targetname 'StormLibDAS'
    filter 'configurations:Release'
        targetname 'StormLibRAS'
