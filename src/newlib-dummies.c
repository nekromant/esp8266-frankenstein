/* These newlib stubs are borrowed and adapted from
    http://caves.org/section/commelect/DUSI/openmag/src/hacks/lpc21xx/com/
    Original copyright notice follows
*/
/************************************************************************/
/* Copyright 2003/12/27 Aeolus Development				*/
/* All rights reserved.							*/
/*									*/
/* Redistribution and use in source and binary forms, with or without	*/
/* modification, are permitted provided that the following conditions	*/
/* are met:								*/
/* 1. Redistributions of source code must retain the above copyright	*/
/*   notice, this list of conditions and the following disclaimer.	*/
/* 2. Redistributions in binary form must reproduce the above copyright	*/
/*   notice, this list of conditions and the following disclaimer in the*/
/*   documentation and/or other materials provided with the 		*/
/*   distribution.							*/
/* 3. The name of the Aeolus Development or its contributors may not be	*/
/* used to endorse or promote products derived from this software 	*/
/* without specific prior written permission.				*/
/*									*/
/* THIS SOFTWARE IS PROVIDED BY THE AEOULUS DEVELOPMENT "AS IS" AND ANY	*/
/* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE	*/
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR	*/
/* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AEOLUS DEVELOPMENT BE	*/
/* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR	*/
/* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF	*/
/* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 	*/
/* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,*/
/* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE */
/* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 	*/
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.			*/
/*									*/
/*  Newlib support routine for closing a file or device.  Depends on 	*/
/* underlying drivers to do the actual work.  This just provides a	*/
/* translation layer between the drivers and newlib proper.		*/
/************************************************************************/



#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include <mem.h>
#include <console.h>

/************************** _close_r ************************************/
/*  Support function.  Closes a file.					*/
/*  struct _reent *r	-- re-entrancy structure, used by newlib to 	*/
/*			support multiple threads of operation.		*/
/*  int file		-- number referring to the open file. Generally	*/
/*			obtained from a corresponding call to open.	*/
/*  Returns 0 if successful.  Otherwise the error code may be found in 	*/
/* errno.								*/
int _close_r(
	struct _reent *r,
	int file)
{

	/*  First check that we are not being asked to close stdin, 	*/
	/* stdout or stderr.						*/
	if( (file == STDIN_FILENO) || (file == STDOUT_FILENO) || (file == STDERR_FILENO)) {
		return -1;
	}

	return 0;
}


	/**** Locally used variables. ****/
extern char _bss_end[];              /*  end is set in the linker command 	*/
                                     /* file and is the end of statically 	*/
				     /* allocated data (thus start of heap).	*/

static char *heap_ptr;		/* Points to current end of the heap.	*/


/************************** _sbrk_r *************************************/
/*  Support function.  Adjusts end of heap to provide more memory to	*/
/* memory allocator. Simple and dumb with no sanity checks.		*/
/*  struct _reent *r	-- re-entrancy structure, used by newlib to 	*/
/*			support multiple threads of operation.		*/
/*  ptrdiff_t nbytes	-- number of bytes to add.			*/
/*  Returns pointer to start of new heap area.				*/
/*  Note:  This implementation is not thread safe (despite taking a	*/
/* _reent structure as a parameter).  					*/
/*  Since _s_r is not used in the current implementation, the following	*/
/* messages must be suppressed.						*/
/*lint -esym(715, _s_r) not refernced					*/
/*lint -esym(818, _s_r) could be const					*/
void * _sbrk_r(
	struct _reent *_s_r,
	ptrdiff_t nbytes)
{
	char  *base;		/*  errno should be set to  ENOMEM on error	*/
	errno = 0;
	if (!heap_ptr){	/*  Initialize if first time through.		*/
		heap_ptr = _bss_end;
	}
	base = heap_ptr;	/*  Point to end of heap.			*/
	heap_ptr += nbytes;	/*  Increase heap.				*/

	return base;		/*  Return pointer to start of new heap area.	*/
}


_off_t _lseek_r(
    struct _reent *r,
    int file,
    _off_t ptr,
    int dir)
{
	return 0;
}

int _fstat_r(
    struct _reent *r,
    int file,
    struct stat *st)
{

	/*  Always set as character device.				*/
 st->st_mode = S_IFCHR;	/*lint !e960 !e632 !e915 octal constant 	*/
			/* assigned to strong type with implicit 	*/
			/* signed/unsigned conversion.  Required by 	*/
			/* newlib.					*/
 return 0;
}

double __ieee754_sqrt(double x) {
	return x;
}

void  * _malloc_r(void *ctx, size_t sz) {
	return (void *) os_malloc(sz);
}

void *_realloc_r(void *ctx, void *ptr, size_t sz) {
	return (void *)os_realloc(ptr, sz);
}

void _free_r(void *ctx, void *ptr)
{
	return (void *) os_free(ptr);
}


_ssize_t  _read_r (void *ptr, int fd, char *buf, ssize_t cnt)
{
	return cnt;
}

_ssize_t _write_r (void *ptr, int fd, char *buf, ssize_t cnt)
{
	return cnt;
}

void abort()
{

}


void *malloc(size_t sz) {
    return (void *) os_malloc(sz);
}

void *realloc(void *oldptr, size_t sz) {
    return (void *) os_realloc(oldptr, sz);
}


void _calloc_r(void *ctx, size_t sz, int num_el) {
    return (void *) os_calloc(sz * num_el);
}

void calloc(size_t sz, int num_el) {
    return (void *) os_calloc(sz * num_el);
}

void free(void *ptr) {
    return os_free(ptr);
}
