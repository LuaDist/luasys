-- Lua Internet Server Application: ISAPI Extension Launcher

local root = ...

-- Set C-modules placement
do
    local cpath = root:gsub([[^\\%?\]], "")

    package.cpath = cpath .. "\\lua5.1.dll;"
	.. cpath .. "\\?51.dll;"
	.. cpath .. "\\?.dll;"
	.. package.cpath
end


local sys = require("sys")

sys.thread.init()


local function process(ecb)
    local path = ecb:getvar"PATH_TRANSLATED"
    local chunk, err = loadfile(path)
    if err then error(err) end
    chunk(ecb)
end

return process
