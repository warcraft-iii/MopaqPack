-- xmake.lua
-- @Author : Dencer (tdaddon@163.com)
-- @Link   : https://dengsir.github.io
-- @Date   : 4/19/2019, 12:55:13 PM

target('StormLib')
    set_kind('static')
    set_languages('cxx17')

    add_files(
        '../../StormLib/src/**.c',
        '../../StormLib/src/**.cpp'
    )

    add_headerfiles(
        '../../StormLib/src/**.h'
    )

    del_files(
        '../../StormLib/src/adpcm/*_old.*',
        '../../StormLib/src/pklib/crc32.c',
        '../../StormLib/src/zlib/compress.c'
    )

    if is_mode('release') then
        set_basename('StormLibRAS')
    end

    if is_mode('debug') then
        set_basename('StormLibDAS')
    end

    if is_os('windows') then
        add_defines('_LIB')
    end

