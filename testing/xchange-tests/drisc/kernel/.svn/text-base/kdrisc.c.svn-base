/*
 *   kdrisc.c
 *
 *   Copyright (C) 2002 Christian Poellabauer
 *
 *   Cleanup and modifications by Ivan B. Ganev, 2003
 *
 */

/* 
 *   Kernel-loadable module for event channel support
 */

/* Required definitions. */
#define MODULE
#ifndef __KERNEL__
#define __KERNEL__
#endif

#include "config.h"

/* Various required header files. */
#include <linux/config.h> 
#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h> 
#include <linux/tty.h>
#include <linux/unistd.h>
#include <linux/socket.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/semaphore.h>
#include "kdrisc.h"

#ifdef KPLUGINS_INTEGRATION
#include <linux/kp_dcg_iface.h>
#endif

MODULE_AUTHOR("Eisenhauer/Poellabauer/Ganev");
MODULE_DESCRIPTION("DRISC Module");
MODULE_LICENSE("GPL");

/* Global variables. */
frec_p *frhead;
frec_p *frecs;
frec_p *orhead;
addrs_t memptr;
pid_t dpid;
DECLARE_MUTEX(kdrisc_sem);
DECLARE_MUTEX(memory_sem);

/* ceiling function. */
int ceil(int x)
{
  if ((x % ALIGN) == 0)
	  return (x);
  else
	  return ((x/ALIGN)*ALIGN+ALIGN);
}

/* Memory management. */

addrs_t DKmallocMM (addrs_t memptr, size_t size, int priority) {

  memptr = vmalloc(size);

  frhead = (frec_p *)memptr;
  frecs  = (frec_p *)(memptr + sizeof(frec_p *));
  orhead = (frec_p *)(memptr + 2*sizeof(frec_p *));

  return memptr;
}

void DKfreeMM (addrs_t memptr) {

  vfree(memptr);
}

void DInitMM (addrs_t memptr, size_t size) {
  
  frec_p frp;
  int maxentries;
  unsigned long aligner;

  *frhead = (frec_p) (memptr + size/2);  
  *orhead = NULL;
  
  maxentries = (size/2)/sizeof(struct frec) - 10;

  for (frp = *frhead; frp < *frhead + maxentries; frp++) 
    frp->next = frp + 1;

  (*frhead)->next = NULL;
  (*frhead + maxentries - 1)->next = NULL;
  *frecs = *frhead + 1;
  (*frhead)->fbp = memptr + 3*sizeof(frec_p *) + 1;
  aligner = (unsigned long) ((*frhead)->fbp);
  aligner = aligner%ALIGN;
  (*frhead)->fbp += (ALIGN - aligner);
  (*frhead)->size = size/2 - 3*sizeof(frec_p *) - 1 - (ALIGN - aligner);
}

addrs_t DAllocMM (size_t size) {

  frec_p frp, prev_frp;
  addrs_t frstart;

  prev_frp = frp = *frhead;

  down(&memory_sem);
  while (frp) {
    if (frp->size >= ceil(size)) {

      frstart = frp->fbp;
      frp->fbp += ceil(size);
      frp->size -= ceil(size);

      if (DInsertEntry(frstart, size) == -1) {
        up(&memory_sem);
        return NULL;
      }
      if (frp->size) {
        up(&memory_sem);
	return (frstart);
      }
      
      DDelRecord (prev_frp, frp);
      up(&memory_sem);
      return (frstart);

    }
    prev_frp = frp;
    frp = frp->next;
  }
  up(&memory_sem);
  return (addrs_t)NULL;
}

void DDelRecord (frec_p prev_frp, frec_p frp) {

  if (frp == *frhead)
    *frhead = frp->next;
  else
    prev_frp->next = frp->next;

  frp->next = *frecs;
  *frecs = frp;
}

void DFreeMM (addrs_t addr) {

  size_t size;
  frec_p frp, new_frp, prev_frp = NULL;
 
  down(&memory_sem);
  size = DFindEntry(addr);

  if (size == 0) {
    up(&memory_sem);
    return;
  }
 
  if ((new_frp = *frecs) == NULL) {
    up(&memory_sem);
    return;
  }

  new_frp->fbp = addr;
  new_frp->size = ceil(size);
  *frecs = new_frp->next;
  frp = *frhead;

  if (frp == NULL || addr <= frp->fbp) {
    new_frp->next = frp;
    *frhead = new_frp;
    DMergeRecords (new_frp);
    DRemoveEntry(addr);
    up(&memory_sem);
    return;
  }

  while (frp && addr > frp->fbp) {
    prev_frp = frp;
    frp = frp->next;
  }

  new_frp->next = prev_frp->next;
  prev_frp->next = new_frp;
  DMergeRecords (prev_frp);

  DRemoveEntry(addr);
  up(&memory_sem);
}

int DInsertEntry (addrs_t addr, size_t size) {

  frec_p n_frp;

  if ((n_frp = *frecs) == NULL) {
    return -1;
  }

  n_frp->fbp = addr;
  n_frp->size = ceil(size);
  *frecs = n_frp->next;
  n_frp->next = *orhead;
  *orhead = n_frp;

  return 0;
}

int DFindEntry (addrs_t addr) {

  frec_p frp;

  frp = *orhead;

  while (frp) {
    if (frp->fbp == addr)
      return frp->size;
    frp = frp->next;
  }

  return 0;
}

void DRemoveEntry (addrs_t addr) {

  frec_p frp, prev_frp;
  
  frp = *orhead;
  prev_frp = *orhead;

  while (frp) {
    if (frp->fbp == addr) {
      if (frp == *orhead)
        *orhead = frp->next;
      else
        prev_frp->next = frp->next;

      frp->next = *frecs;
      *frecs = frp;
      return;
    }
    prev_frp = frp;
    frp = frp->next;
  }
}

addrs_t DReallocMM (addrs_t oldmem, size_t bytes) {

  size_t size;
  addrs_t newmem;

  if ((size = DFindEntry(oldmem)) == 0)
    return NULL;

  DFreeMM(oldmem);

  newmem = DAllocMM(bytes);

  if (bytes > size)
    memcpy(newmem, oldmem, size);
  else
    memcpy(newmem, oldmem, bytes);

  return(newmem);
}

void DMergeRecords (frec_p frp) {

  frec_p next_frp;

  if ((next_frp = frp->next) == NULL)
    return;

  if (frp->fbp + frp->size == next_frp->fbp) {
    frp->size += next_frp->size;
    DDelRecord (frp, next_frp);
  }
  else
    frp = next_frp;

  if ((next_frp = frp->next) == NULL)
    return;

  if (frp->fbp + frp->size == next_frp->fbp) {
    frp->size += next_frp->size;
    DDelRecord (frp, next_frp);
  }
}

int sys_gethostname(char *name, int len)
{
  int i, errno;

  if (len < 0)
    return -EINVAL;
  down_read(&uts_sem);
  i = 1 + strlen(system_utsname.nodename);
  if (i > len)
    i = len;
  errno = 0;
  memcpy(name, system_utsname.nodename, i);
  up_read(&uts_sem);
  return errno;
}

/* strdup function */
char *strdup(char *s)
{
  char *p;

  p = (char *)kmalloc(strlen(s)+1, GFP_KERNEL);
  if (p != NULL)
    strcpy(p, s);
  return p;
}

extern void *sys_call_table[];
int (*sys_exit)(int error_code);

/* Initialize module. */
static int __init kdrisc_init(void) 
{
  sys_exit = sys_call_table[__NR_exit];

  /* Allocate free pages. */
  memptr = DKmallocMM(memptr, MEM_SIZE, MEM_PRIORITY);

  /* Initialize kernel memory. */
  DInitMM(memptr, MEM_SIZE);

#ifdef KPLUGINS_INTEGRATION
  /* Setup iface hooks into kernel plugins */
  kp_dr_clone_code	= dr_clone_code;
  kp_dr_code_size	= dr_code_size;
#endif

  return 0;
}

/* Remove module. */
static void __exit kdrisc_cleanup(void) 
{
  /* Free memory pages. */
  DKfreeMM(memptr);

#ifdef KPLUGINS_INTEGRATION 
  /* Unhook iface from kernel plugins */
  kp_dr_clone_code	= NULL;
  kp_dr_code_size	= NULL;
#endif
}

module_init(kdrisc_init);
module_exit(kdrisc_cleanup); 

