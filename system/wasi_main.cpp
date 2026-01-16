// Copyright 2023-2025 Leaning Technologies

#include <cstdarg>
#include <sysexits.h>
#include "wasi_api.h"
#include "impl.h"

extern "C" {

static uint8_t argv_buf[32*1024];
static char* argv[1024];
void __syscall_main_args(int* argc_p, char*** argv_p)
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

static uint8_t environ_buf[32*1024];
static char* cheerp_environ[1024];
extern "C" char ** environ;
void __syscall_main_environ()
{
	__wasi_errno_t err;

	// Get the sizes of the arrays we'll have to create to copy in the args.
	size_t environ_buf_size;
	size_t environ_size;
	err = __wasi_environ_sizes_get(&environ_size, &environ_buf_size);
	if (err != __WASI_ERRNO_SUCCESS) {
		__wasi_proc_exit(EX_OSERR);
	}

	// Add 1 for the NULL pointer to mark the end, and check for overflow.
	size_t num_ptrs = environ_size + 1;
	if (num_ptrs == 0) {
		__wasi_proc_exit(EX_SOFTWARE);
	}
	if (num_ptrs > 1024 || environ_buf_size > 32*1024) {
		__wasi_proc_exit(EX__MAX);
	}

	err = __wasi_environ_get(reinterpret_cast<uint8_t **>(cheerp_environ), reinterpret_cast<uint8_t*>(environ_buf));
	if (err != __WASI_ERRNO_SUCCESS) {
		__wasi_proc_exit(EX_OSERR);
	}
	environ = cheerp_environ;
}

} // extern "C"

namespace sys_internal {
long futex_wrapper(uint32_t* uaddr, int futex_op, va_list args)
{
	return futex(uaddr, futex_op, true, args);
}

} // namespace sys_internal
