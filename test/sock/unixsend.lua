#!/usr/bin/env lua

local sys = require"sys"
local sock = require"sys.sock"


local file = "/tmp/test_sock_lua"

local fd = sock.handle()
assert(fd:socket("dgram", "unix"))

local addr = assert(sock.addr_un(file))
assert(fd:connect(addr))

fd:send(sys.stdin:read(13))
