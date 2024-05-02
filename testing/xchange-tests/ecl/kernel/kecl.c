/*
 *   kecl.c
 *
 *   Copyright (C) 2002 Christian Poellabauer
 *
 *   Cleanup and modifications by Ivan B. Ganev, 2003
 */

/* 
 *   Kernel-loadable module for event channel support
 */

#include "config.h"

/* Required definitions. */
#ifdef LINUX_KERNEL_MODULE
#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif
#endif

/* Various required header files. */
#include <linux/config.h> 
#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h> 
#include <linux/types.h> 
#include <linux/tty.h>
#include <linux/unistd.h>
#include <linux/socket.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/semaphore.h>
#include "kecl.h"

#include "ecl.h"

#ifdef KPLUGINS_INTEGRATION
#include <linux/kp_dcg_iface.h>
#endif

MODULE_AUTHOR("Eisenhauer/Poellabauer/Ganev");
MODULE_DESCRIPTION("ECL Module");
MODULE_LICENSE("GPL");

__kernel_ssize_t (*sys_kread) (unsigned int fd, char *buf, size_t count);

/* Initialize module. */
static int __init kecl_init(void) 
{
  
#ifdef KPLUGINS_INTEGRATION
  kp_new_ecl_parse_context		= new_ecl_parse_context;
  kp_ecl_assoc_externs			= ecl_assoc_externs;
  kp_ecl_parse_for_context		= ecl_parse_for_context;
  kp_ecl_subroutine_declaration		= ecl_subroutine_declaration;
  kp_ecl_code_gen			= ecl_code_gen;
  kp_ecl_code_free			= ecl_code_free;
  kp_ecl_free_parse_context		= ecl_free_parse_context;

  kp_dcg_present			= 1;

  printk(KERN_INFO "ecl: Linked Ecode interface into kp\n");
#endif

  return 0;
}

/* Remove module. */
static void __exit kecl_cleanup(void) 
{
#ifdef KPLUGINS_INTEGRATION
  kp_new_ecl_parse_context		= NULL;
  kp_ecl_assoc_externs			= NULL;
  kp_ecl_parse_for_context		= NULL;
  kp_ecl_subroutine_declaration		= NULL;
  kp_ecl_code_gen			= NULL;
  kp_ecl_code_free			= NULL;
  kp_ecl_free_parse_context		= NULL;

  kp_dcg_present			= 0;

  printk(KERN_INFO "ecl: Unlinked Ecode interface from kp\n");
#endif
}

module_init(kecl_init);
module_exit(kecl_cleanup); 

