
-- add modes: debug and release
add_rules('mode.debug', 'mode.release')

if is_mode('release') then
    add_cxflags('/MT')
end

if is_mode('debug') then
    add_cxflags('/MTd')
end

add_subdirs(
    '3rd/xmake/StormLib'
)

target('MopaqPack')

    set_kind('binary')
    set_languages('cxx17')

    add_files('src/*.cpp')

    add_includedirs('3rd/StormLib/src')

    add_deps('StormLib')
