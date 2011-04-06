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
	SET(WIN32_SRC src/win32/sys_win32.c )
	SET(LIBS winmm ws2_32 mswsock)
ELSEIF(UNIX)
	SET(LIBS pthread)
ENDIF(WIN32 AND NOT CYGWIN)

ADD_LUA_MODULE(sys src/luasys.c src/thread/sys_thread.c src/sock/sys_sock.c src/mem/sys_mem.c src/event/evq.c src/event/timeout.c ${WIN32_SRC})
TARGET_LINK_LIBRARIES(sys ${LIBS})

# Install all files and documentation
INSTALL (TARGETS sys DESTINATION ${INSTALL_CMOD})

INSTALL (FILES README VERSION DESTINATION ${INSTALL_DATA})
INSTALL (DIRECTORY test/ DESTINATION ${INSTALL_TEST})
