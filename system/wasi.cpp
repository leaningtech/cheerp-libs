extern "C" {
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <sysexits.h>
#include <fcntl.h>
}
#include <limits.h>
#include <limits>
#include <ctime>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdlib>

#include "wasi_api.h"

#include "impl.h"

namespace sys_internal {

double timezone_offset()
{
	return 0;
}
double real_time_now()
{
	uint64_t time;
	__wasi_errno_t err = __wasi_clock_time_get(__WASI_CLOCKID_REALTIME, 0, &time);
	return err? std::numeric_limits<double>::quiet_NaN() : time;
}
double monotonic_time_now()
{
	uint64_t time;
	__wasi_errno_t err = __wasi_clock_time_get(__WASI_CLOCKID_MONOTONIC, 0, &time);
	return err? std::numeric_limits<double>::quiet_NaN() : time;
}
double cpu_time_now()
{
	uint64_t time;
	__wasi_errno_t err = __wasi_clock_time_get(__WASI_CLOCKID_PROCESS_CPUTIME_ID, 0, &time);
	return err? std::numeric_limits<double>::quiet_NaN() : time;
}

void killAllThreads()
{
	// This function is empty, there are no threads to close in WASI mode.
}

} //namespace sys_internal

namespace {

int rootFd = -1;

struct DirData {
	int fd;
	__wasi_dircookie_t cookie;
	DirData* next;

	static DirData* opened;

	static void add(int fd)
	{
		DirData* newD = static_cast<DirData*>(malloc(sizeof(DirData)));
		newD->fd = fd;
		newD->cookie = 0;
		newD->next = opened;
		opened = newD;
	}
	static void remove(int fd)
	{
		DirData* prev = nullptr;
		for(DirData* d = opened; d != nullptr; d = d->next)
		{
			if (d->fd != fd)
				continue;
			if (prev)
			{
				prev->next = d->next;
			}
			else
			{
				opened = d->next;
			}
			free(d);
		}
	}
	static DirData* get(int fd)
	{
		for(DirData* d = opened; d != nullptr; d = d->next)
		{
			if (d->fd == fd)
				return d;
		}
		return nullptr;
	}
};

}

extern "C" {

long WEAK __syscall_writev(long fd, const iovec* ios, long len)
{
	static_assert(sizeof(iovec) == sizeof(__wasi_iovec_t), "incompatible iovec size");
	__wasi_size_t ret;
	__wasi_errno_t err = __wasi_fd_write(fd, reinterpret_cast<const __wasi_ciovec_t*>(ios), len, &ret);
	return err? -err : ret;
}

long WEAK __syscall_write(int fd, void* buf, long count)
{
	__wasi_ciovec_t ios { reinterpret_cast<uint8_t*>(buf), __wasi_size_t(count) };
	__wasi_size_t ret;
	__wasi_errno_t err = __wasi_fd_write(fd, &ios, 1, &ret);
	return err? -err : ret;
}

long WEAK __syscall_open(const char* pathname, int flags, ...)
{
	if (rootFd < 0)
	{
		return -EPERM;
	}
	if (pathname[0] == '/')
		pathname++;


    __wasi_oflags_t oflags = 0;
	if (flags & O_CREAT)
		oflags |= __WASI_OFLAGS_CREAT;
	if (flags & O_DIRECTORY)
		oflags |= __WASI_OFLAGS_DIRECTORY;
	if (flags & O_EXCL)
		oflags |= __WASI_OFLAGS_EXCL;
	if (flags & O_TRUNC)
		oflags |= __WASI_OFLAGS_TRUNC;
	__wasi_fdflags_t fdflags = 0;
	if (flags & O_SYNC)
		fdflags |= __WASI_FDFLAGS_SYNC;
	if (flags & O_RSYNC)
		fdflags |= __WASI_FDFLAGS_RSYNC;
	if (flags & O_DSYNC)
		fdflags |= __WASI_FDFLAGS_DSYNC;
	if (flags & O_NONBLOCK)
		fdflags |= __WASI_FDFLAGS_NONBLOCK;
	if (flags & O_APPEND)
		fdflags |= __WASI_FDFLAGS_APPEND;
	int openmode = flags & O_ACCMODE;
	__wasi_rights_t rights = 0b11111111111111111111111111111;
	rights &= ~(__WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_READ |
		  __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_ALLOCATE |
		  __WASI_RIGHTS_FD_READDIR | __WASI_RIGHTS_FD_FILESTAT_SET_SIZE);
	if (openmode == O_RDONLY || openmode == O_RDWR)
		rights |= __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_READDIR;
	if (openmode == O_WRONLY || openmode == O_RDWR)
	{
		 rights |=
			 __WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_WRITE |
			 __WASI_RIGHTS_FD_ALLOCATE |
			 __WASI_RIGHTS_FD_FILESTAT_SET_SIZE;
	}

	__wasi_lookupflags_t lookup_flags = 0;
	if ((flags & O_NOFOLLOW) == 0)
		lookup_flags |= __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW;
	__wasi_rights_t fs_rights_base = rights;
	__wasi_rights_t fs_rights_inheriting = fs_rights_base;
	__wasi_fd_t ret;
	__wasi_errno_t err = __wasi_path_open(rootFd, lookup_flags, pathname, oflags, fs_rights_base, fs_rights_inheriting, fdflags, &ret);
	if (err == 0 && (flags&O_DIRECTORY))
	{
		DirData::add(ret);
	}
	return err? -err : ret;
}

long WEAK __syscall_close(int fd)
{
	DirData::remove(fd);
	return __wasi_fd_close(fd);
}

// NOTE: Internal musl definition
struct statx
{
	uint32_t stx_mask;
	uint32_t stx_blksize;
	uint64_t stx_attributes;
	uint32_t stx_nlink;
	uint32_t stx_uid;
	uint32_t stx_gid;
	uint16_t stx_mode;
	uint16_t pad1;
	uint64_t stx_ino;
	uint64_t stx_size;
	uint64_t stx_blocks;
	uint64_t stx_attributes_mask;
	struct
	{
		int64_t tv_sec;
		uint32_t tv_nsec;
		int32_t pad;
	} stx_atime, stx_btime, stx_ctime, stx_mtime;
	uint32_t stx_rdev_major;
	uint32_t stx_rdev_minor;
	uint32_t stx_dev_major;
	uint32_t stx_dev_minor;
	uint64_t spare[14];
};
int WEAK __syscall_statx(int dirfd, const char* pathname, int flags, int mask, statx* st)
{
	__wasi_lookupflags_t wflags = 0;
	__wasi_filestat_t wst;
	__wasi_errno_t err = 0;
	if ((flags & AT_SYMLINK_NOFOLLOW) == 0)
		wflags |= __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW;
	memset(st, 0, sizeof(struct statx));
	// TODO: Actually support dirfd, for now we assume it's just fstat
	if(dirfd >= 0)
	{
		err = __wasi_fd_filestat_get(dirfd, &wst);
	}
	else
	{
		if(pathname[0] == '/')
			pathname++;
		err = __wasi_path_filestat_get(rootFd, wflags, pathname, &wst);
	}
	st->stx_size = wst.size;
	switch (wst.filetype) {
		case __WASI_FILETYPE_BLOCK_DEVICE:
			st->stx_mode |= S_IFBLK;
			break;
		case __WASI_FILETYPE_CHARACTER_DEVICE:
			st->stx_mode |= S_IFCHR;
			break;
		case __WASI_FILETYPE_DIRECTORY:
			st->stx_mode |= S_IFDIR;
			break;
		case __WASI_FILETYPE_REGULAR_FILE:
			st->stx_mode |= S_IFREG;
			break;
		case __WASI_FILETYPE_SOCKET_DGRAM:
		case __WASI_FILETYPE_SOCKET_STREAM:
			st->stx_mode |= S_IFSOCK;
			break;
		case __WASI_FILETYPE_SYMBOLIC_LINK:
			st->stx_mode |= S_IFLNK;
			break;
	}
	if(st->stx_mode == 0)
	{
		err = ENOENT;
	}
	return -err;
}

int WEAK __syscall_access(const char *pathname, int mode)
{
	if (pathname[0] == '/')
		pathname = pathname + 1;
	// Check for target file existence.
	// TODO: when we support multiple preopened fds, also look at the permissions
	// on the parent fd
	__wasi_lookupflags_t lookup_flags = __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW;
	__wasi_filestat_t file;
	__wasi_errno_t err = __wasi_path_filestat_get(rootFd, lookup_flags, pathname, &file);
	return err? -err : 0;
}

int WEAK __syscall_link(const char *oldpath, const char *newpath)
{
	assert(oldpath[0] == '/');
	oldpath++;
	assert(newpath[0] == '/');
	newpath++;
	__wasi_errno_t err = __wasi_path_symlink(oldpath, rootFd, newpath);
	return err? -err : 0;
}

int WEAK __syscall_rename(const char *oldpath, const char *newpath)
{
	assert(oldpath[0] == '/');
	oldpath++;
	assert(newpath[0] == '/');
	newpath++;
	__wasi_errno_t err = __wasi_path_rename(rootFd, oldpath, rootFd, newpath);
	return err? -err : 0;
}

int WEAK __syscall_unlink(const char *pathname)
{
	if(pathname[0] == '/')
		pathname++;
	__wasi_errno_t err = __wasi_path_unlink_file(rootFd, pathname);
	return err? -err : 0;
}

struct linux_dirent64 {
	ino64_t        d_ino;    /* 64-bit inode number */
	off64_t        d_off;    /* 64-bit offset to next structure */
	unsigned short d_reclen; /* Size of this dirent */
	unsigned char  d_type;   /* File type */
	char           d_name[]; /* Filename (null-terminated) */
};

DirData* DirData::opened = nullptr;

int WEAK __syscall_getdents64(int fd, void* dir, int count)
{
	static_assert(sizeof(__wasi_dirent_t) >= sizeof(linux_dirent64), "wrong assumption");
	size_t used = 0;
	__wasi_errno_t err = 0;
	__wasi_size_t size = 0;
	DirData* data = DirData::get(fd);
	if (dir == nullptr)
		return -ENOENT;
	__wasi_dircookie_t cookie = data->cookie;
	uint8_t* buf = static_cast<uint8_t*>(dir);
	err = __wasi_fd_readdir(fd, buf, count, cookie, &size);
	if (err)
		return -err;
	size_t reado = 0;
	size_t writeo = 0;
	__wasi_dirent_t tmp;
	// Make sure a whole entry can still fit
	while(reado + sizeof(__wasi_dirent_t) <= size)
	{
		memcpy(&tmp, buf+reado, sizeof(tmp));
		// Make sure the whole name could fit in the buffer
		size_t nextReadOffset = reado + sizeof(__wasi_dirent_t) + tmp.d_namlen;
		if(nextReadOffset > size)
			break;
		linux_dirent64* tmpdst = reinterpret_cast<linux_dirent64*>(buf+writeo);
		tmpdst->d_ino = 0;
		tmpdst->d_off = tmp.d_next;
		data->cookie = tmp.d_next;
		tmpdst->d_reclen = sizeof(linux_dirent64)+tmp.d_namlen;
		tmpdst->d_type = tmp.d_type;
		memmove(&tmpdst->d_name, buf + reado + sizeof(__wasi_dirent_t), tmp.d_namlen);
		tmpdst->d_name[tmp.d_namlen] = '\0';
		reado = nextReadOffset;
		writeo += sizeof(linux_dirent64) + tmp.d_namlen;
	}

	return err? -err : writeo;
}

int WEAK __syscall_mkdir(const char *pathname, int mode)
{
	if(pathname[0] == '/')
		pathname++;
	__wasi_errno_t err = __wasi_path_create_directory(rootFd, pathname);
	return err? -err : 0;
}

size_t WEAK __syscall__llseek(unsigned int fd, unsigned long offset_high, unsigned long offset_low,
	unsigned long long* result, unsigned int whence)
{
	__wasi_filedelta_t offset = uint64_t(offset_high) << 32 | offset_low;
	__wasi_filesize_t ret;
	__wasi_errno_t err = __wasi_fd_seek(fd, offset, whence, &ret);
	*result = ret;
	return -err;
}

int WEAK __syscall_read(int fd, void* buf, int count)
{
	__wasi_iovec_t ios { reinterpret_cast<uint8_t*>(buf), __wasi_size_t(count) };
	__wasi_size_t ret;
	__wasi_errno_t err = __wasi_fd_read(fd, &ios, 1, &ret);
	return err? -err : ret;
}
long WEAK __syscall_readv(long fd, const iovec* ios, long len)
{
	static_assert(sizeof(iovec) == sizeof(__wasi_iovec_t), "incompatible iovec size");
	__wasi_size_t ret;
	__wasi_errno_t err = __wasi_fd_read(fd, reinterpret_cast<const __wasi_iovec_t*>(ios), len, &ret);
	return err? -err : ret;
}

// NOTE: numbers from 101 upwards are reserved for C++ constructors
// 11 is chosen as a low number but after potentially more core initialization
__attribute__((constructor(11)))
void __preopen_root_fd()
{
	__wasi_prestat_t prestat;
	__wasi_errno_t ret = __wasi_fd_prestat_get(3, &prestat);
	if (ret == __WASI_ERRNO_BADF)
		return;
	if (ret != __WASI_ERRNO_SUCCESS)
		__wasi_proc_exit(EX_OSERR);
	switch (prestat.tag)
	{
		case __WASI_PREOPENTYPE_DIR:
		{
			rootFd = 3;
			break;
		}
		default:
		{
			break;
		}
	}
}

long WEAK __syscall_exit(int code)
{
	__wasi_proc_exit(code);
	return 0;
}

void __cxa_throw_wasm_adapter(void *thrown_object, void *tinfo, void (*dest)(void *))
{
	const char msg[] = "Exception raised. Aborting...\n";
	__syscall_write(1, (void*)(msg), sizeof(msg));
	__wasi_proc_exit(EX_OSERR);
}

bool testUseAtomicWait()
{
	return true;
}

} // extern "C"

namespace std {

__attribute__((noreturn))
__attribute__((nothrow))
void
terminate() noexcept
{
	const char msg[] = "std::terminate() called. Exiting...\n";
	__syscall_write(1, (void*)(msg), sizeof(msg));
	__wasi_proc_exit(EX_OSERR);
}

}

namespace __sanitizer {
using uptr = unsigned int;
bool IsWasi() { return true; }
// Stub out genericjs functions
void InitEnv() {}
uptr GetCallstack(uptr *dest, uptr dest_len, uptr skip) {}
uptr GetUtf16FunctionNameAtPc(uptr pc, char16_t *dest, uptr len) {}
} // namespace __sanitizer
