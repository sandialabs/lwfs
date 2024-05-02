/*
 *   kdrisc.c
 *
 *   Copyright (C) 2002 Christian Poellabauer
 *
 *   Cleanup and modifications by Ivan B. Ganev, 2003
 *
 */

/* Required definitions. */
#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif

#define kdrisc_h 1

/* Various required header files. */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/kbd_ll.h>
#include <linux/types.h> 
#include <linux/utsname.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/segment.h> 
#include <asm/signal.h> 

#define MEM_SIZE (unsigned) (1<<21)
#define MEM_PRIORITY GFP_KERNEL
#define ALIGN 8

extern struct semaphore kdrisc_sem;

extern char * getenv(const char *name);

extern int    (*sys_exit)(int error_code);

typedef struct frec *frec_p;
typedef char *addrs_t;
typedef void *any_t;

typedef struct frec {
  addrs_t fbp;			/* Free block pointer. */
  size_t size;
  frec_p next;
} frec_t;			/* Free record type. */

/* Global variables. */
extern frec_p *frhead;
extern frec_p *frecs;
extern frec_p *orhead;
extern addrs_t p_memptr;

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

