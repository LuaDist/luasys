# Copyright (C) 2007-2009 LuaDist.
# Created by Peter Kapec
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

PROJECT(luasys C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
INCLUDE(dist.cmake)

# Build
INCLUDE_DIRECTORIES(src src/event)

IF(WIN32 AND NOT CYGWIN)
	ADD_DEFINITIONS(-DWIN32)
	#need? SET(WIN32_SRC src/win32/sys_win32.c )
	SET(LIBS winmm ws2_32 mswsock)
ELSEIF(UNIX)
	SET(LIBS pthread rt)
	ADD_DEFINITIONS(-DUSE_EPOLL)
ENDIF(WIN32 AND NOT CYGWIN)



ADD_LUA_MODULE(sys
  src/luasys.c
# Note: these are included by luasys.c:
#  src/sys_comm.c
#  src/sys_date.c
#  src/sys_env.c
#  src/sys_evq.c
#  src/sys_file.c
#  src/sys_fs.c
#  src/sys_log.c
#  src/sys_proc.c
#  src/sys_rand.c
#  src/sys_unix.c
#  src/thread/sys_thread.c
#  src/thread/thread_dpool.c
#  src/thread/thread_msg.c
#  src/thread/thread_sync.c
#  src/mem/sys_mem.c
#  src/mem/membuf.c
#  src/event/evq.c
#  src/event/epoll.c
#  src/event/kqueue.c
#  src/event/poll.c
#  src/event/select.c
#  src/event/signal.c
#  src/event/timeout.c
  src/sock/sys_sock.c
)
TARGET_LINK_LIBRARIES(sys ${LIBS})

# Install all files and documentation
INSTALL (TARGETS sys DESTINATION ${INSTALL_CMOD})

INSTALL (FILES README VERSION DESTINATION ${INSTALL_DATA})
INSTALL (DIRECTORY test/ DESTINATION ${INSTALL_TEST})
