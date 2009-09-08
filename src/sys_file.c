/* Lua System: File I/O */

#ifdef _WIN32

#include <stdio.h>	/* _fileno, P_tmpdir */
#include <io.h>		/* _open_osfhandle */

#define O_RDONLY	GENERIC_READ
#define O_WRONLY	GENERIC_WRITE
#define O_RDWR		(GENERIC_READ | GENERIC_WRITE)

#else

#if defined(__linux__)
#define O_FSYNC		O_SYNC
#endif

static const int fdopt_flags[] = {
    O_CREAT, O_EXCL, O_TRUNC, O_APPEND, O_NONBLOCK, O_NOCTTY, O_FSYNC
};
static const char *const fdopt_names[] = {
    "creat", "excl", "trunc", "append", "nonblock", "noctty", "sync",
    NULL
};

#endif


/*
 * Returns: fd_udata
 */
static int
sys_file (lua_State *L)
{
    lua_boxinteger(L, -1);
    luaL_getmetatable(L, FD_TYPENAME);
    lua_setmetatable(L, -2);
    return 1;
}

/*
 * Arguments: fd_udata, pathname (string), [mode (string: "r", "w", "rw"),
 *	permissions (number), options (string) ...]
 * Returns: [fd_udata]
 */
static int
sys_open (lua_State *L)
{
    fd_t fd, *fdp = checkudata(L, 1, FD_TYPENAME);
    const char *pathname = luaL_checkstring(L, 2);
    const char *mode = lua_tostring(L, 3);
#ifndef _WIN32
    mode_t perm = (mode_t) lua_tointeger(L, 4);
#else
    int append = 0;
#endif
    int flags = O_RDONLY, i;

#undef OPT_START
#define OPT_START	5

    if (mode) {
	switch (mode[0]) {
	case 'w': flags = O_WRONLY; break;
	case 'r': if (mode[1] == 'w') flags = O_RDWR;
	}
    }
#ifndef _WIN32
    for (i = lua_gettop(L); i >= OPT_START; --i) {
	flags |= fdopt_flags[luaL_checkoption(L, i, NULL, fdopt_names)];
    }

    sys_vm_leave();
    fd = open(pathname, flags, perm);
    sys_vm_enter();
#else
    {
	DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD creation = OPEN_EXISTING;
	DWORD attr = FILE_ATTRIBUTE_NORMAL
	 | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION;

	for (i = lua_gettop(L); i >= OPT_START; --i) {
	    const char *opt = lua_tostring(L, i);
	    if (opt)
		switch (opt[0]) {
		case 'c':	/* creat */
		    creation &= ~OPEN_EXISTING;
		    creation |= CREATE_ALWAYS;
		    break;
		case 'e':	/* excl */
		    share = 0;
		    break;
		case 't':	/* trunc */
		    creation &= ~OPEN_EXISTING;
		    creation |= TRUNCATE_EXISTING;
		    break;
		case 'a':	/* append */
		    append = 1;
		    break;
		case 's':	/* sync */
		    attr |= FILE_FLAG_WRITE_THROUGH;
		    break;
		case 'r':	/* random access */
		    attr |= FILE_FLAG_RANDOM_ACCESS;
		    break;
		}
	}

	sys_vm_leave();
	fd = CreateFile(pathname, flags, share, NULL, creation, attr, NULL);
	sys_vm_enter();
    }
#endif
    if (fd != (fd_t) -1) {
	*fdp = fd;
#ifdef _WIN32
	if (append) {
	    SetFilePointer(fd, 0, NULL, SEEK_END);
	}
#endif
	lua_settop(L, 1);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata, pathname (string), [permissions (number)]
 * Returns: [fd_udata]
 */
static int
sys_create (lua_State *L)
{
    fd_t fd, *fdp = checkudata(L, 1, FD_TYPENAME);
    const char *pathname = luaL_checkstring(L, 2);
#ifndef _WIN32
    mode_t perm = (mode_t) lua_tointeger(L, 3);
#endif

    sys_vm_leave();
#ifndef _WIN32
    fd = creat(pathname, perm);
#else
    fd = CreateFile(pathname, O_WRONLY, FILE_SHARE_READ | FILE_SHARE_WRITE,
     NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL
     | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, NULL);
#endif
    sys_vm_enter();

    if (fd != (fd_t) -1) {
	*fdp = fd;
	lua_settop(L, 1);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata, [prefix (string), auto-remove (boolean)]
 * Returns: [filename]
 */
static int
sys_tempfile (lua_State *L)
{
    fd_t fd, *fdp = checkudata(L, 1, FD_TYPENAME);
    const char *prefix = lua_tostring(L, 2);
    int auto_remove = lua_isboolean(L, -1) && lua_toboolean(L, -1);
    char path[MAX_PATH + 1];

#ifndef _WIN32
    static const char template[] = "XXXXXX";
    const char *tmpdir;
    size_t len, pfxlen = lua_strlen(L, 2);

    if (!(tmpdir = getenv("TMPDIR"))
     && !(tmpdir = getenv("TMP"))
     && !(tmpdir = getenv("TEMP")))
	tmpdir = P_tmpdir;
    len = strlen(tmpdir);
    if (len + 1 + pfxlen + sizeof(template) > sizeof(path))
	return 0;
    memcpy(path, tmpdir, len);
    if (path[len - 1] != '/')
	path[len++] = '/';
    if (pfxlen) {
	memcpy(path + len, prefix, pfxlen);
	len += pfxlen;
    }
    memcpy(path + len, template, sizeof(template));  /* include term. zero */

    sys_vm_leave();
    fd = mkstemp(path);
    sys_vm_enter();
#else
    char tmpdir[MAX_PATH + 1];

    if (!GetTempPath(MAX_PATH, tmpdir)
     || !GetTempFileName(tmpdir, prefix, 0, path))
	goto err;

    sys_vm_leave();
    fd = CreateFile(path, GENERIC_READ | GENERIC_WRITE,
     0, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS
     | FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_HIDDEN
     | (auto_remove ? FILE_FLAG_DELETE_ON_CLOSE : 0), NULL);
    sys_vm_enter();
#endif
    if (fd != (fd_t) -1) {
	*fdp = fd;
#ifndef _WIN32
	if (auto_remove) unlink(path);
#endif
	lua_pushstring(L, path);
	return 1;
    }
#ifdef _WIN32
 err:
#endif
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata (reading), fd_udata (writing)
 * Returns: fd_udata (reading)
 */
static int
sys_pipe (lua_State *L)
{
    fd_t *rfdp = checkudata(L, 1, FD_TYPENAME);
    fd_t *wfdp = checkudata(L, 2, FD_TYPENAME);

#ifndef _WIN32
    fd_t filedes[2];
    if (!pipe(filedes)) {
	*rfdp = filedes[0];
	*wfdp = filedes[1];
#else
    if (CreatePipe(rfdp, wfdp, NULL, 0)) {
#endif
	lua_settop(L, 1);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata, [close_std_handle (boolean)]
 * Returns: [boolean]
 */
static int
sys_close (lua_State *L)
{
    fd_t *fdp = checkudata(L, 1, FD_TYPENAME);
    const int close_std = lua_toboolean(L, 2);

    if (*fdp != (fd_t) -1) {
	luaL_getmetatable(L, FD_TYPENAME);
	lua_rawgeti(L, -1, (int) *fdp);  /* don't close std. handles */
	if (!(lua_isnil(L, -1) || close_std))
	    lua_pushboolean(L, 0);
	else {
#ifndef _WIN32
	    int res;
	    do res = close(*fdp);
	    while (res == -1 && SYS_ERRNO == EINTR);
	    lua_pushboolean(L, !res);
#else
	    lua_pushboolean(L, CloseHandle(*fdp));
#endif
	    *fdp = (fd_t) -1;
	}
	return 1;
    }
    return 0;
}

/*
 * Arguments: fd_udata
 */
static int
sys_dispose (lua_State *L)
{
    fd_t *fdp = checkudata(L, 1, FD_TYPENAME);
    *fdp = (fd_t) -1;
    return 0;
}

/*
 * Arguments: fd_udata, stream (string: "in", "out", "err")
 * Returns: [fd_udata]
 */
static int
sys_set_std (lua_State *L)
{
    fd_t fd = (fd_t) lua_unboxinteger(L, 1, FD_TYPENAME);
    const char *stream = luaL_checkstring(L, 2);
    int dst;

#ifndef _WIN32
    int res;

    dst = (*stream == 'i') ? STDIN_FILENO
     : (*stream == 'o') ? STDOUT_FILENO : STDERR_FILENO;
    do res = dup2(fd, dst);
    while (res == -1 && SYS_ERRNO == EINTR);
    if (res != -1) {
#else
    dst = (*stream == 'i') ? STD_INPUT_HANDLE
     : (*stream == 'o') ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE;
    if (SetStdHandle(dst, fd)) {
#endif
	lua_settop(L, 1);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata, offset (number),
 *	[whence (string: "set", "cur", "end")]
 * Returns: offset
 */
static int
sys_seek (lua_State *L)
{
    fd_t fd = (fd_t) lua_unboxinteger(L, 1, FD_TYPENAME);
    const lua_Number offset = lua_tonumber(L, 2);
    int64_t off = (int64_t) offset;  /* to avoid warning */
    const char *whencep = lua_tostring(L, 3);
    int whence = SEEK_CUR;

    /* SEEK_* and FILE_* (win32) are equal */
    if (whencep) {
	switch (whencep[0]) {
	case 's': whence = SEEK_SET; break;
	case 'e': whence = SEEK_END; break;
	}
    }
#ifndef _WIN32
    off = lseek(fd, off, whence);
#else
    {
	LONG off_hi = INT64_HIGH(off);
	LONG off_lo = INT64_LOW(off);

	off_lo = SetFilePointer(fd, off_lo, &off_hi, whence);
	off = (off_lo == -1L && SYS_ERRNO != NO_ERROR) ? (int64_t) -1
	 : INT64_MAKE(off_lo, off_hi);
    }
#endif
    if (off != (int64_t) -1) {
	lua_pushnumber(L, (lua_Number) off);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata, offset (number)
 * Returns: [fd_udata]
 */
static int
sys_set_end (lua_State *L)
{
    fd_t fd = (fd_t) lua_unboxinteger(L, 1, FD_TYPENAME);
    const lua_Number offset = lua_tonumber(L, 2);
    const int64_t off = (int64_t) offset;  /* to avoid warning */
    int res;

#ifndef _WIN32
    do res = ftruncate(fd, off);
    while (res == -1 && SYS_ERRNO == EINTR);
    if (!res) {
#else
    {
	LONG off_hi = INT64_HIGH(off);
	LONG off_lo = INT64_LOW(off);
	LONG cur_hi = 0L, cur_lo = 0L;

	cur_lo = SetFilePointer(fd, 0L, &cur_hi, SEEK_CUR);
	SetFilePointer(fd, off_lo, &off_hi, SEEK_SET);
	res = SetEndOfFile(fd);
	SetFilePointer(fd, cur_lo, &cur_hi, SEEK_SET);
    }
    if (res) {
#endif
	lua_settop(L, 1);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata, offset (number), length (number),
 *	[lock/unlock (boolean)]
 * Returns: [fd_udata]
 */
static int
sys_lock (lua_State *L)
{
    fd_t fd = (fd_t) lua_unboxinteger(L, 1, FD_TYPENAME);
    const lua_Number offset = lua_tonumber(L, 2);
    const int64_t off = (int64_t) offset;  /* to avoid warning */
    const lua_Number length = lua_tonumber(L, 3);
    const int64_t len = (int64_t) length;  /* to avoid warning */
    const int locking = lua_isboolean(L, -1) && lua_toboolean(L, -1);
    int res;

#ifndef _WIN32
    struct flock lock;

    lock.l_type = locking ? (F_RDLCK | F_WRLCK) : F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = off;
    lock.l_len = len;

    sys_vm_leave();
    do res = fcntl(fd, F_SETLK, &lock);
    while (res == -1 && SYS_ERRNO == EINTR);
    sys_vm_enter();

    if (res != -1) {
#else
    sys_vm_leave();
    {
	const DWORD off_hi = INT64_HIGH(off);
	const DWORD off_lo = INT64_LOW(off);
	const DWORD len_hi = INT64_HIGH(len);
	const DWORD len_lo = INT64_LOW(len);

	res = locking ? LockFile(fd, off_lo, off_hi, len_lo, len_hi)
	 : UnlockFile(fd, off_lo, off_hi, len_lo, len_hi);
    }
    sys_vm_enter();

    if (res) {
#endif
	lua_settop(L, 1);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata, [mode (string)]
 * Returns: [file_udata]
 */
static int
sys_to_file (lua_State *L)
{
    fd_t *fdp = checkudata(L, 1, FD_TYPENAME);
    const char *mode = luaL_optstring(L, 2, "r");
    FILE *f;

#ifndef _WIN32
    f = fdopen((int) *fdp, mode);
#else
    f = _fdopen(_open_osfhandle((long) *fdp, 0), mode);
#endif
    if (f) {
	lua_boxpointer(L, f);
	luaL_getmetatable(L, LUA_FILEHANDLE);
	lua_setmetatable(L, -2);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata, file_udata
 * Returns: [fd_udata]
 */
static int
sys_from_file (lua_State *L)
{
    fd_t *fdp = checkudata(L, 1, FD_TYPENAME);
    FILE **fp = checkudata(L, 2, LUA_FILEHANDLE);

#ifndef _WIN32
    *fdp = fileno(*fp);
#else
    *fdp = (fd_t) _get_osfhandle(_fileno(*fp));
#endif
    *fp = NULL;
    return 1;
}

/*
 * Arguments: fd_udata, {string | membuf_udata} ...
 * Returns: [success/partial (boolean), count (number)]
 */
static int
sys_write (lua_State *L)
{
    fd_t fd = (fd_t) lua_unboxinteger(L, 1, FD_TYPENAME);
    ssize_t n = 0;  /* number of chars actually write */
    int i, nargs = lua_gettop(L);

    for (i = 2; i <= nargs; ++i) {
	struct sys_buffer sb;
	int nw;

	if (!sys_buffer_read_init(L, i, &sb))
	    continue;
	sys_vm_leave();
#ifndef _WIN32
	do nw = write(fd, sb.ptr.r, sb.size);
	while (nw == -1 && SYS_ERRNO == EINTR);
#else
	{
	    DWORD l;
	    nw = WriteFile(fd, sb.ptr.r, sb.size, &l, NULL) ? l : -1;
	}
#endif
	sys_vm_enter();
	if (nw == -1) {
	    if (n > 0 || SYS_ERRNO == EAGAIN) break;
	    return sys_seterror(L, 0);
	}
	n += nw;
	sys_buffer_read_next(&sb, nw);
	if ((size_t) nw < sb.size) break;
    }
    lua_pushboolean(L, (i > nargs));
    lua_pushinteger(L, n);
    return 2;
}

/*
 * Arguments: fd_udata, [membuf_udata, count (number)]
 * Returns: [string | count (number) | false (EAGAIN)]
 */
static int
sys_read (lua_State *L)
{
    fd_t fd = (fd_t) lua_unboxinteger(L, 1, FD_TYPENAME);
    size_t n = !lua_isnumber(L, -1) ? ~((size_t) 0)
     : (size_t) lua_tointeger(L, -1);
    const size_t len = n;  /* how much total to read */
    size_t rlen;  /* how much to read */
    int nr;  /* number of bytes actually read */
    struct sys_buffer sb;
    char buf[SYS_BUFSIZE];

    sys_buffer_write_init(L, 2, &sb, buf, sizeof(buf));
    do {
	rlen = (n <= sb.size) ? n : sb.size;
	sys_vm_leave();
#ifndef _WIN32
	do nr = read(fd, sb.ptr.w, rlen);
	while (nr == -1 && SYS_ERRNO == EINTR);
#else
	{
	    DWORD l;
	    nr = ReadFile(fd, sb.ptr.w, rlen, &l, NULL) ? l : -1;
	}
#endif
	sys_vm_enter();
	if (nr == -1) break;
	n -= nr;  /* still have to read `n' bytes */
    } while ((n != 0L && nr == (int) rlen)  /* until end of count or eof */
     && sys_buffer_write_next(L, &sb, buf, 0));
    if (nr <= 0 && len == n) {
	if (!nr || SYS_ERRNO != EAGAIN) goto err;
	lua_pushboolean(L, 0);
    } else {
	if (!sys_buffer_write_done(L, &sb, buf, nr))
	    lua_pushinteger(L, len - n);
    }
    return 1;
 err:
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata
 * Returns: [fd_udata]
 */
static int
sys_flush (lua_State *L)
{
    fd_t fd = (fd_t) lua_unboxinteger(L, 1, FD_TYPENAME);
    int res;

    sys_vm_leave();
#ifndef _WIN32
    res = fsync(fd);
#else
    res = !FlushFileBuffers(fd);
#endif
    sys_vm_enter();

    if (!res) {
	lua_settop(L, 1);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata, nonblocking (boolean)
 * Returns: [fd_udata]
 */
static int
sys_nonblocking (lua_State *L)
{
    fd_t fd = (fd_t) lua_unboxinteger(L, 1, FD_TYPENAME);
    const int nonblocking = lua_toboolean(L, 2);

#ifndef _WIN32
    int flags = fcntl(fd, F_GETFL);

    if (!fcntl(fd, F_SETFL, nonblocking ? flags | O_NONBLOCK
     : flags ^ O_NONBLOCK)) {
#else
    size_t mode = nonblocking ? 0 : MAILSLOT_WAIT_FOREVER;

    if (SetMailslotInfo(fd, mode)) {
#endif
	lua_settop(L, 1);
	return 1;
    }
    return sys_seterror(L, 0);
}

/*
 * Arguments: fd_udata
 * Returns: string
 */
static int
sys_tostring (lua_State *L)
{
    fd_t fd = (fd_t) lua_unboxinteger(L, 1, FD_TYPENAME);

    if (fd != (fd_t) -1)
	lua_pushfstring(L, FD_TYPENAME " (%d)", (int) fd);
    else
	lua_pushliteral(L, FD_TYPENAME " (closed)");
    return 1;
}


static luaL_reg fd_meth[] = {
    {"open",		sys_open},
    {"create",		sys_create},
    {"tempfile",	sys_tempfile},
    {"pipe",		sys_pipe},
    {"close",		sys_close},
    {"dispose",		sys_dispose},
    {"set_std",		sys_set_std},
    {"seek",		sys_seek},
    {"set_end",		sys_set_end},
    {"lock",		sys_lock},
    {"to_file",		sys_to_file},
    {"from_file",	sys_from_file},
    {"write",		sys_write},
    {"read",		sys_read},
    {"flush",		sys_flush},
    {"nonblocking",	sys_nonblocking},
    {"__tostring",	sys_tostring},
    {"comm_init",	sys_comm_init},
    {"comm_control",	sys_comm_control},
    {"comm_timeout",	sys_comm_timeout},
    {"comm_queues",	sys_comm_queues},
    {"comm_purge",	sys_comm_purge},
    {"__gc",		sys_close},
    {SYS_BUFIO_META,	NULL},  /* can operate with buffers */
    {NULL, NULL}
};
