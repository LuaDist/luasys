
LUA= /tmp/lua-5.1.4

T= /usr/local/lib/lua/5.1/sys.so

COPT= -O2 -fPIC -DNDEBUG
CWARNS = -W -Wall -Wextra -pedantic \
	-Waggregate-return \
	-Wbad-function-cast \
	-Wcast-align \
	-Wcast-qual \
	-Wdeclaration-after-statement \
	-Wdisabled-optimization \
	-Wmissing-prototypes \
	-Wnested-externs \
	-Wpointer-arith \
	-Wshadow \
	-Wsign-compare \
	-Wstrict-prototypes \
	-Wundef \
	-Wwrite-strings \
#	-Wunreachable-code

CFLAGS= $(COPT) $(CWARNS) $(MYCFLAGS) -I$(LUA)/src
LIBS= -lpthread

MYCFLAGS=
MYLDFLAGS= -shared -Bdynamic

CC= /usr/bin/gcc
MAKE_DLL= /usr/bin/ld $(MYLDFLAGS) -o
STRIP= /usr/bin/strip
RM= rm -f

OBJS= luasys.o thread/sys_thread.o sock/sys_sock.o mem/sys_mem.o \
    event/evq.o event/timeout.o isa/fcgi/sys_fcgi.o
LDOBJS= $(OBJS)

all: $(T)

$(T): $(OBJS)
	$(MAKE_DLL) $@ $(LDOBJS) $(LIBS)
	$(STRIP) $@

bsd:
	$(MAKE) all MYCFLAGS="-DUSE_KQUEUE" LDOBJS="*.o"

osx:
	$(MAKE) all MYCFLAGS="-DUSE_KQUEUE" STRIP="echo strip" \
	MYLDFLAGS="-bundle -undefined dynamic_lookup"

clean:
	$(RM) $(T) *.o */*.o */*/*.o


luasys.o: sys_comm.c sys_date.c sys_env.c sys_evq.c sys_file.c sys_fs.c \
    sys_proc.c sys_rand.c sys_unix.c \
    common.h event/evq.h event/timeout.h
thread/sys_thread.o: thread/sys_thread.c thread/thread_dpool.c \
    thread/thread_msg.c thread/thread_sync.c common.h
sock/sys_sock.o: sock/sys_sock.c common.h
mem/sys_mem.o: mem/sys_mem.c mem/membuf.c common.h
event/evq.o: event/evq.c event/epoll.c event/kqueue.c event/poll.c \
    event/select.c event/signal.c \
    event/epoll.h event/kqueue.h event/poll.h event/select.h \
    common.h event/evq.h event/timeout.h
event/timeout.o: event/timeout.c common.h event/evq.h event/timeout.h
isa/fcgi/sys_fcgi.o: isa/fcgi/sys_fcgi.c common.h
