#!/usr/bin/env lua

local sys = require("sys")
local sock = require("sys.sock")


print"-- sys.handle <-> io.file"
do
    local fd = sys.handle()
    assert(fd:create("test", 384))
    assert(fd:write"fd <")

    local file = fd:to_file"w"
    assert(file:write"> file")
    file:flush()

    fd:from_file(file)
    fd:close()
    sys.remove"test"
    print("OK")
end


print"-- Logs"
do
    local log = assert(sys.log("Lua.Log"))
    assert(log:error("Error"):warn("Warning"))
    print("OK")
end


print"-- Random"
do
    local rand = assert(sys.random())
    for i = 1, 20 do
	sys.stdout:write(rand(100), "; ")
    end
    print("\nOK")
end


print"-- Emulate popen()"
do
    local fdi, fdo = sys.handle(), sys.handle()
    assert(fdi:pipe(fdo))
    local s = "test pipe"
    assert(sys.spawn("lua",
	{'-l', 'sys', '-e', 'sys.stdout:write[[' .. s .. ']]'},
	nil, nil, fdo))
    fdo:close()
    assert(fdi:read() == s)
    fdi:close()
    print("OK")
end


print"-- SocketPair"
do
    local fdi, fdo = sock.handle(), sock.handle()
    assert(fdi:socket(fdo))
    local s = "test socketpair"
    assert(fdo:write(s))
    fdo:close()
    assert(fdi:read() == s)
    fdi:close()
    print("OK")
end


print"-- Directory list"
do
    for file, type in sys.dir('.') do
	print(file, type and "DIR" or "FILE")
    end
    print("OK")
end


print"-- Signal: wait SIGINT"
do
    local function on_signal(evq, evid, _, _, _, timeout)
	if timeout then
	    assert(evq:timeout(evid))
	    assert(evq:ignore_signal("INT", false))
	    print"SIGINT enabled. Please, press Ctrl-C..."
	else
	    print"Thanks!"
	end
    end

    local evq = assert(sys.event_queue())

    assert(evq:add_signal("INT", on_signal, 3000, true))
    assert(evq:ignore_signal("INT", true))

    evq:loop(10000)
    print"OK"
end

