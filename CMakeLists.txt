# Copyright (C) 2007-2012 LuaDist.
# Created by Peter Kapec
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

project ( luasys C )
cmake_minimum_required ( VERSION 2.8 )
include ( cmake/dist.cmake )
include ( lua )


include_directories ( src src/event )

if ( WIN32 AND NOT CYGWIN )
  add_definitions ( -DWIN32 )
  #need? SET(WIN32_SRC src/win32/sys_win32.c )
  set ( LIBS winmm ws2_32 mswsock )
elseif ( UNIX )
set ( LIBS rt )
add_definitions ( -DUSE_EPOLL )
endif ( )
#FIX: BSD and OSX? -DUSE_KQUEUE

install_lua_module ( sys src/luasys.c # Note: these are included by luasys.c:
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
  src/sock/sys_sock.c LINK ${LIBS} )
install_data ( README VERSION )
install_test ( test/ )
