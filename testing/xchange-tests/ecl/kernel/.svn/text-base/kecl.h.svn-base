/*
 *   kecl.c
 *
 *   Copyright (C) 2002 Christian Poellabauer
 *
 *   Cleanup and modifications by Ivan B. Ganev, 2003
 */

/* Required definitions. */
#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif

#define kecl_h 1

/* Various required header files. */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/signal.h> 

/* Stdlib functions. */
extern char *strdup(const char *s);
/*
 * 2.4.19 as of today (12 Jan 2004) has simple_strtol() not strtol()
 */
#define strtol(x...)   simple_strtol(x)
extern int   atoi(const char *nptr);

extern int   (*sys_exit)(int error_code);
extern __kernel_ssize_t (*sys_kread) (unsigned int fd, char *buf, size_t count);

typedef struct frec * frec_p;
typedef char *        addrs_t;
typedef void *        any_t;

typedef struct frec {
  addrs_t fbp;			/* Free block pointer. */
  size_t size;
  frec_p next;
} frec_t;			/* Free record type. */

/* Prototypes. */
extern addrs_t DKmallocMM (addrs_t p_memptr, size_t size, int priority);
extern addrs_t DAllocMM (size_t size);
extern void    DKfreeMM (addrs_t p_memptr);
extern void    DMergeRecords (frec_p frp);
extern void    DDelRecord (frec_p prev_frp, frec_p frp);
extern void    DInitMM (addrs_t p_memptr, size_t size);
extern addrs_t DReallocMM (addrs_t oldmem, size_t bytes);
extern int     DInsertEntry (addrs_t addr, size_t size);
extern int     DFindEntry (addrs_t addr);
extern void    DRemoveEntry (addrs_t addr);
extern void    DFreeMM (addrs_t addr);
