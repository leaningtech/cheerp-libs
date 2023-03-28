extern "C" {
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <fcntl.h>
}
#include <limits.h>
#include <errno.h>
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
	errno = err;
	return time;
}
double monotonic_time_now()
{
	uint64_t time;
	errno = __wasi_clock_time_get(__WASI_CLOCKID_MONOTONIC, 0, &time);
	return time;
}
double cpu_time_now()
{
	uint64_t time;
	errno = __wasi_clock_time_get(__WASI_CLOCKID_PROCESS_CPUTIME_ID, 0, &time);
	return time;
}

} //namespace sys_internal

namespace {

int rootFd = -1;

}

extern "C" {

long WEAK __syscall_writev(long fd, const iovec* ios, long len)
{
	static_assert(sizeof(iovec) == sizeof(__wasi_iovec_t), "incompatible iovec size");
	__wasi_size_t ret;
	__wasi_errno_t err = __wasi_fd_write(fd, reinterpret_cast<const __wasi_ciovec_t*>(ios), len, &ret);
	errno = err;
	return err? -1 : ret;
}

long WEAK __syscall_write(int fd, void* buf, long count)
{
	__wasi_ciovec_t ios { reinterpret_cast<uint8_t*>(buf), __wasi_size_t(count) };
	__wasi_size_t ret;
	__wasi_errno_t err = __wasi_fd_write(fd, &ios, 1, &ret);
	errno = err;
	return err? -1 : ret;
}

long WEAK __syscall_open(const char* pathname, int flags, ...)
{
	if (rootFd < 0)
	{
		errno = EPERM;
		return -1;
	}
	if (pathname[0] == '/')
		pathname = pathname + 1;
	int dirflags = __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW;
		__wasi_rights_t fs_rights_base = 0b11111111111111111111111111111;
		__wasi_rights_t fs_rights_inheriting = fs_rights_base;
		__wasi_fdflags_t fdflags = 0;
		__wasi_fd_t ret;
	errno = __wasi_path_open(rootFd, dirflags, pathname, flags, fs_rights_base, fs_rights_inheriting, fdflags, &ret);
	return errno? -1 : 0;

}
long WEAK __syscall_close(int fd)
{
	errno = __wasi_fd_close(fd);
	return errno? -1 : 0;
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
int __syscall_statx(int dirfd, const char* pathname, int flags, int mask, statx* st)
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
		err = __wasi_path_filestat_get(dirfd, wflags, pathname, &wst);
	}
	else
	{
		assert(pathname[0] == '/');
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
	errno = err;
	return err? -1 : 0;
}

int __syscall_link(const char *oldpath, const char *newpath)
{
	assert(oldpath[0] == '/');
	oldpath++;
	assert(newpath[0] == '/');
	newpath++;
	__wasi_errno_t err = __wasi_path_symlink(oldpath, rootFd, newpath);
	errno = err;
	return err? -1 : 0;
}

int __syscall_rename(const char *oldpath, const char *newpath)
{
	assert(oldpath[0] == '/');
	oldpath++;
	assert(newpath[0] == '/');
	newpath++;
	__wasi_errno_t err = __wasi_path_rename(rootFd, oldpath, rootFd, newpath);
	errno = err;
	return err? -1 : 0;
}

int __syscall_unlink(const char *pathname)
{
	assert(pathname[0] == '/');
	pathname++;
	__wasi_errno_t err = __wasi_path_unlink_file(rootFd, pathname);
	errno = err;
	return err? -1 : 0;
}

struct linux_dirent64 {
	ino64_t        d_ino;    /* 64-bit inode number */
	off64_t        d_off;    /* 64-bit offset to next structure */
	unsigned short d_reclen; /* Size of this dirent */
	unsigned char  d_type;   /* File type */
	char           d_name[]; /* Filename (null-terminated) */
};
int __syscall_getdents64(int fd, void* dir, int count)
{
	static_assert(sizeof(__wasi_dirent_t) >= sizeof(linux_dirent64), "wrong assumption");
	size_t used = 0;
	__wasi_errno_t err = 0;
	__wasi_size_t size = 0;
	__wasi_dircookie_t cookie = 0;
	uint8_t* buf = static_cast<uint8_t*>(dir);
	err = __wasi_fd_readdir(fd, buf, count, 0, &size);
	size_t read = 0;
	__wasi_dirent_t tmp;
	while(read < size)
	{
		memcpy(&tmp, buf+read, sizeof(tmp));
		linux_dirent64* tmpdst = reinterpret_cast<linux_dirent64*>(buf+read);
		tmpdst->d_ino = 0;
		tmpdst->d_off = tmp.d_next;
		tmpdst->d_reclen = sizeof(linux_dirent64)+tmp.d_namlen;
		tmpdst->d_type = tmp.d_type;
		memmove(&tmpdst->d_name, buf + read + sizeof(tmp), tmp.d_namlen);
		read += sizeof(tmp) + tmp.d_namlen;
	}

	errno = err;
	return err? -1 : read;
}

int __syscall_mkdir(const char *pathname, int mode)
{
	assert(pathname[0] == '/');
	pathname++;
	__wasi_errno_t err = __wasi_path_create_directory(rootFd, pathname);
	errno = err;
	return err? -1 : 0;
}

size_t __syscall__llseek(unsigned int fd, unsigned long offset_high, unsigned long offset_low,
	unsigned long long* result, unsigned int whence)
{
    __wasi_filedelta_t offset = uint64_t(offset_high) << 32 | offset_low;
    __wasi_filesize_t ret;
	__wasi_errno_t err = __wasi_fd_seek(fd, offset, whence, &ret);
	errno = err;
	return err? -1 : ret;
}

int __syscall_read(int fd, void* buf, int count)
{
	__wasi_iovec_t ios { reinterpret_cast<uint8_t*>(buf), __wasi_size_t(count) };
	__wasi_size_t ret;
	__wasi_errno_t err = __wasi_fd_read(fd, &ios, 1, &ret);
	errno = err;
	return err? -1 : ret;
}
long __syscall_readv(long fd, const iovec* ios, long len)
{
	static_assert(sizeof(iovec) == sizeof(__wasi_iovec_t), "incompatible iovec size");
	__wasi_size_t ret;
	__wasi_errno_t err = __wasi_fd_read(fd, reinterpret_cast<const __wasi_iovec_t*>(ios), len, &ret);
	errno = err;
	return err? -1 : ret;
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

static uint8_t argv_buf[32*1024];
static char* argv[1024];
void WEAK __syscall_main_args(int* argc_p, char*** argv_p)
{
	__wasi_errno_t err;

		// Get the sizes of the arrays we'll have to create to copy in the args.
	size_t argv_buf_size;
	size_t argc;
	err = __wasi_args_sizes_get(&argc, &argv_buf_size);
	if (err != __WASI_ERRNO_SUCCESS) {
		__wasi_proc_exit(EX_OSERR);
	}

	// Add 1 for the NULL pointer to mark the end, and check for overflow.
	size_t num_ptrs = argc + 1;
	if (num_ptrs == 0) {
		__wasi_proc_exit(EX_SOFTWARE);
	}
	if (num_ptrs > 1024 || argv_buf_size > 32*1024) {
	__wasi_proc_exit(EX__MAX);
	}

	//// Allocate memory for storing the argument chars.
	//char *argv_buf = static_cast<char*>(malloc(argv_buf_size));
	//if (argv_buf == NULL) {
	//	__wasi_proc_exit(EX_SOFTWARE);
	//}

	//// Allocate memory for the array of pointers. This uses `calloc` both to
	//// handle overflow and to initialize the NULL pointer at the end.
	//char** argv = static_cast<char**>(calloc(num_ptrs, sizeof(char *)));
	//if (argv == NULL) {
	//	free(argv_buf);
	//	__wasi_proc_exit(EX_SOFTWARE);
	//}

	// Fill the argument chars, and the argv array with pointers into those chars.
	err = __wasi_args_get(reinterpret_cast<uint8_t **>(argv), reinterpret_cast<uint8_t*>(argv_buf));
	if (err != __WASI_ERRNO_SUCCESS) {
		//free(argv_buf);
		//free(argv);
		__wasi_proc_exit(EX_OSERR);
	}
	*argc_p = argc;
	*argv_p = argv;
}

long WEAK __syscall_exit(int code)
{
	__wasi_proc_exit(code);
	return 0;
}

void __cxa_throw_wasm(void *thrown_object, void *tinfo, void (*dest)(void *))
{
	const char msg[] = "Exception raised. Aborting...\n";
	__syscall_write(1, (void*)(msg), sizeof(msg));
	__wasi_proc_exit(EX_OSERR);
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
