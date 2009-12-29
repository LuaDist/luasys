/* System library for Lua */

#include "common.h"

#include <sys/stat.h>


#ifdef _WIN32

#define mode_t	int
#define ssize_t	DWORD

int is_WinNT;

#else

#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/sysctl.h>

#define MAX_PATH	260

#endif


static const int sig_flags[] = {
    SIGHUP, SIGINT, SIGQUIT, SIGTERM
};

static const char *const sig_names[] = {
    "HUP", "INT", "QUIT", "TERM", NULL
};


/*
 * Arguments: ..., [number]
 * Returns: string
 */
static int
sys_strerror (lua_State *L)
{
    const int err = luaL_optint(L, -1, SYS_ERRNO);
#ifndef _WIN32
    const char *s = strerror(err);
#else
    char s[256];

    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
     s, sizeof(s), NULL)) {
	char *cp = strrchr(s, '\r');
	if (cp) *cp = '\0';
    } else
	sprintf(s, "Unknown error %i", err);
#endif
    lua_pushstring(L, s);
    return 1;
}

/*
 * Returns: nil, string
 */
int
sys_seterror (lua_State *L, int err)
{
    if (err != 0) {
#ifndef _WIN32
	errno = err;
#else
	SetLastError(err);
#endif
    }
    lua_pushnil(L);
    sys_strerror(L);
    lua_pushvalue(L, -1);
    lua_setglobal(L, SYS_ERROR_MESSAGE);
    return 2;
}


/*
 * Returns: number_of_processors (number)
 */
static int
sys_nprocs (lua_State *L)
{
#ifndef _WIN32
    int n = 1;
#if defined(_SC_NPROCESSORS_ONLN)
    n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n == -1) n = 1;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int mib[2];
    size_t len = sizeof(int);

    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    sysctl(mib, 2, &n, &len, NULL, 0);
#endif
    lua_pushinteger(L, n);
#else
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    lua_pushinteger(L, si.dwNumberOfProcessors);
#endif
    return 1;
}

/*
 * Arguments: [number_of_files (number)]
 * Returns: [number_of_files (number)]
 */
static int
sys_limit_nfiles (lua_State *L)
{
#ifndef _WIN32
    struct rlimit rlim;

    if (lua_gettop(L)) {
	const int n = lua_tointeger(L, 1);

	rlim.rlim_cur = rlim.rlim_max = n;
	if (!setrlimit(RLIMIT_NOFILE, &rlim))
	    return 1;
    } else {
	if (!getrlimit(RLIMIT_NOFILE, &rlim)) {
	    lua_pushinteger(L, rlim.rlim_max);
	    return 1;
	}
    }
    return sys_seterror(L, 0);
#else
    return 0;
#endif
}

/*
 * Arguments: string
 * Returns: number
 */
static int
sys_toint (lua_State *L)
{
    const char *s = lua_tostring(L, 1);
    int num = 0, sign = 1;

    if (s) {
	if (*s == '+' || (*s == '-' && (sign = -1)))
	    ++s;
	while (*s >= '0' && *s <= '9')
	    num = (num << 3) + (num << 1) + (*s++ & ~'0');
    }
    lua_pushinteger(L, sign * num);
    return 1;
}

/*
 * Arguments: error_handler (function), function, any ...
 * Returns: status (boolean), any ...
 */
static int
sys_xpcall (lua_State *L)
{
    const int status = lua_pcall(L, lua_gettop(L) - 2, LUA_MULTRET, 1);

    lua_pushboolean(L, !status);
    lua_insert(L, 2);
    return lua_gettop(L) - 1;
}


#include "sys_file.c"
#include "sys_date.c"
#include "sys_env.c"
#include "sys_evq.c"
#include "sys_fs.c"
#include "sys_log.c"
#include "sys_proc.c"
#include "sys_rand.c"

#ifndef _WIN32
#include "sys_unix.c"
#endif


static luaL_reg syslib[] = {
    {"strerror",	sys_strerror},
    {"nprocs",		sys_nprocs},
    {"limit_nfiles",	sys_limit_nfiles},
    {"toint",		sys_toint},
    {"xpcall",		sys_xpcall},
    DATE_METHODS,
    ENV_METHODS,
    EVQ_METHODS,
    FD_METHODS,
    FS_METHODS,
    LOG_METHODS,
    PROC_METHODS,
    RAND_METHODS,
#ifdef UNIX_METHODS
    UNIX_METHODS,
#endif
    {NULL, NULL}
};


/*
 * Arguments: ..., sys_lib (table)
 */
static void
createmeta (lua_State *L)
{
    const int top = lua_gettop(L);
    const struct meta_s {
	const char *tname;
	luaL_reg *meth;
	int is_index;
    } meta[] = {
	{DIR_TYPENAME,		dir_meth,	0},
	{EVQ_TYPENAME,		evq_meth,	1},
	{FD_TYPENAME,		fd_meth,	1},
	{PERIOD_TYPENAME,	period_meth,	1},
	{PID_TYPENAME,		pid_meth,	1},
	{RAND_TYPENAME,		rand_meth,	0},
	{LOG_TYPENAME,		log_meth,	0},
    };
    int i;

    for (i = 0; i < (int) (sizeof(meta) / sizeof(struct meta_s)); ++i) {
	luaL_newmetatable(L, meta[i].tname);
	if (meta[i].is_index) {
	    lua_pushvalue(L, -1);  /* push metatable */
	    lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
	}
	luaL_register(L, NULL, meta[i].meth);
	lua_pop(L, 1);
    }

    /* Predefined file handles */
    luaL_getmetatable(L, FD_TYPENAME);
    {
	const char *std[] = {"stdin", "stdout", "stderr"};
#ifdef _WIN32
	const fd_t std_fd[] = {
	    GetStdHandle(STD_INPUT_HANDLE),
	    GetStdHandle(STD_OUTPUT_HANDLE),
	    GetStdHandle(STD_ERROR_HANDLE)
	};
#endif
	for (i = 3; i--; ) {
#ifndef _WIN32
	    const fd_t fd = i;
#else
	    const fd_t fd = std_fd[i];
#endif
	    lua_pushstring(L, std[i]);
	    lua_boxinteger(L, fd);
	    lua_pushvalue(L, -3);  /* metatable */
	    lua_pushboolean(L, 1);
	    lua_rawseti(L, -2, (int) fd);  /* don't close std. handles */
	    lua_setmetatable(L, -2);
	    lua_rawset(L, top);
	}
    }
    lua_settop(L, top);
}


LUALIB_API int luaopen_sys (lua_State *L);

LUALIB_API int
luaopen_sys (lua_State *L)
{
    luaL_register(L, "sys", syslib);
    createmeta(L);

    luaopen_sys_mem(L);
    luaopen_sys_thread(L);
#ifdef _WIN32
    /* Is Win32 NT platform? */
    {
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	is_WinNT = (GetVersionEx(&osvi)
	 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
    }

    luaopen_sys_win32(L);
#else
    /* Ignore sigpipe or it will crash us */
    signal_set(SIGPIPE, SIG_IGN);
    /* To interrupt blocking syscalls */
    signal_set(SYS_SIGINTR, NULL);
#endif
    return 1;
}
