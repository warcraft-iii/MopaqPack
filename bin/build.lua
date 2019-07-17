-- build.lua
-- @Author : Dencer (tdaddon@163.com)
-- @Link   : https://dengsir.github.io
-- @Date   : 7/13/2019, 9:50:37 PM

local hTemplate = [[
#pragma once
#include "lua.hpp"
void Init%s(lua_State* L, int env);
]]

local cTemplate = [[
#include "lua.hpp"

const unsigned char CODE[] = {
    %s
};

void Init%s(lua_State* L, int env)
{
    luaL_loadbuffer(L, (const char*)CODE, %s, "@%s");
    lua_pushvalue(L, env);
    lua_setupvalue(L, -2, 1);
    lua_call(L, 0, 0);
}
]]

local function writeFile(path, data)
    local file = assert(io.open(path, 'wb'))
    file:write(data)
    file:close()
end

local function upperFirst(word)
    return (word:gsub('^.', string.upper))
end

local function generateFuncName(filePath)
    local r = {}
    for word in filePath:gsub('%.lua$', ''):gmatch('[%w]+') do
        table.insert(r, upperFirst(word))
    end
    return table.concat(r)
end

local function main(input, output)
    local filePath = input:match('src/lua/(.+)$')
    local fileName = input:match('/([^/]+)$')
    local funcName = generateFuncName(filePath)
    local code = string.dump(assert(loadfile(input)))
    local len = #code
    do
        local n = 0
        code = code:gsub('.', function(x)
            local r = string.format('0x%02X', string.byte(x)) .. ', '
            n = n + 1
            if n % 16 == 0 then
                n = 0
                return r .. '\n    '
            else
                return r
            end
        end)
    end
    local h = string.format(hTemplate, funcName)
    local c = string.format(cTemplate, code, funcName, len, filePath)

    writeFile(output .. '/' .. fileName .. '.cpp', c)
    writeFile(output .. '/' .. fileName .. '.h', h)
end

main(arg[1], arg[2])
