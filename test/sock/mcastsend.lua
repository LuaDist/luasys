#!/usr/bin/env lua

local sys = require"sys"
local sock = require"sys.sock"


local MCAST_ADDR = "234.5.6.7"
local MCAST_PORT = 25000

local fd = sock.handle()
assert(fd:socket("dgram"))

assert(fd:sockopt("reuseaddr", 1))
assert(fd:bind(sock.addr_in(MCAST_PORT + 1)))

local addr = sock.addr_in(MCAST_PORT, MCAST_ADDR)

assert(fd:send(sys.stdin:read(13), addr))
