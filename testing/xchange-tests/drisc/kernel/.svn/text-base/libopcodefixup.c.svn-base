/*
 * libopcodefixup.c: a compatibility library to allow us to link
 * statically to libopcode.a and them put the code into the linux
 * kernel...
 *
 * Copyright (c) Ivan B. Ganev, 2003 
 */


/*
 * _setjmp()/longjmp() code was shameless lifted off of KDB's source
 * code, whereas the associated typedefs were shamelessly stolen from
 * the glibc header files (required for compatibility with the
 * (precompiled) libopcodes.a, which we have no control over.
 */
#define JB_BX   0
#define JB_SI   1
#define JB_DI   2
#define JB_BP   3
#define JB_SP   4
#define JB_PC   5

#define JB_SIZE 24

typedef int __jmp_buf[6];

typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;


typedef struct __jmp_buf_tag
{
  __jmp_buf __jmpbuf;
  int __mask_was_saved;
  __sigset_t __saved_mask;
} jmp_buf[1];

/*
 * Needed for abort(). This is defined in other parts of the kernel
 * version of drisc.
 */
extern int (*sys_exit)(int error_code);




/*
 * optimized strchr() implementation shamelessly lifted from the linux
 * kernel.
 */
char * strchr(const char * s, int c)
{
  int d0;
  register char * __res;
  __asm__ __volatile__
    ("movb %%al,%%ah\n"
     "1:\tlodsb\n\t"
     "cmpb %%ah,%%al\n\t"
     "je 2f\n\t"
     "testb %%al,%%al\n\t"
     "jne 1b\n\t"
     "movl $1,%1\n"
     "2:\tmovl %1,%0\n\t"
     "decl %0"
     :"=a" (__res), "=&S" (d0) : "1" (s),"0" (c));
  return __res;
}

/*
 * optimized strcpy() implementation shamelessly lifted from the linux
 * kernel.
 */
char * strcpy(char * dest,const char *src)
{
  int d0, d1, d2;
  __asm__ __volatile__
    ("1:\tlodsb\n\t"
     "stosb\n\t"
     "testb %%al,%%al\n\t"
     "jne 1b"
     : "=&S" (d0), "=&D" (d1), "=&a" (d2)
     :"0" (src),"1" (dest) : "memory");
  return dest;
}

/*
 * optimized strlen() implementation shamelessly lifted from the linux
 * kernel.
 */
unsigned int strlen(const char * s)
{
  int d0;
  register int __res;
  __asm__ __volatile__
    ("repne\n\t"
     "scasb\n\t"
     "notl %0\n\t"
     "decl %0"
     :"=c" (__res), "=&D" (d0) :"1" (s),"a" (0), "0" (0xffffffff));
  return __res;
}

/*
 * _setjmp() code was shameless lifted off of KDB's source code,
 * whereas the associated typedefs were shamelessly stolen from the
 * glibc header files (required for compatibility with the
 * (precompiled) libopcodes.a, which we have no control over.
 */
int _setjmp(jmp_buf jb)
{
#if defined(CONFIG_FRAME_POINTER)
  __asm__ ("movl 8(%esp), %eax\n\t"
	   "movl %ebx, 0(%eax)\n\t"
	   "movl %esi, 4(%eax)\n\t"
	   "movl %edi, 8(%eax)\n\t"
	   "movl (%esp), %ecx\n\t"
	   "movl %ecx, 12(%eax)\n\t"
	   "leal 8(%esp), %ecx\n\t"
	   "movl %ecx, 16(%eax)\n\t"
	   "movl 4(%esp), %ecx\n\t"
	   "movl %ecx, 20(%eax)\n\t");
#else   /* CONFIG_FRAME_POINTER */
  __asm__ ("movl 4(%esp), %eax\n\t"
	   "movl %ebx, 0(%eax)\n\t"
	   "movl %esi, 4(%eax)\n\t"
	   "movl %edi, 8(%eax)\n\t"
	   "movl %ebp, 12(%eax)\n\t"
	   "leal 4(%esp), %ecx\n\t"
	   "movl %ecx, 16(%eax)\n\t"
	   "movl 0(%esp), %ecx\n\t"
	   "movl %ecx, 20(%eax)\n\t");
#endif   /* CONFIG_FRAME_POINTER */
  return 0;
}

/*
 * longjmp() code was shameless lifted off of KDB's source code,
 * whereas the associated typedefs were shamelessly stolen from the
 * glibc header files (required for compatibility with the
 * (precompiled) libopcodes.a, which we have no control over.
 */
void longjmp(jmp_buf jb, int reason)
{
#if defined(CONFIG_FRAME_POINTER)
  __asm__("movl 8(%esp), %ecx\n\t"
	  "movl 12(%esp), %eax\n\t"
	  "movl 20(%ecx), %edx\n\t"
	  "movl 0(%ecx), %ebx\n\t"
	  "movl 4(%ecx), %esi\n\t"
	  "movl 8(%ecx), %edi\n\t"
	  "movl 12(%ecx), %ebp\n\t"
	  "movl 16(%ecx), %esp\n\t"
	  "jmp *%edx\n");
#else    /* CONFIG_FRAME_POINTER */
  __asm__("movl 4(%esp), %ecx\n\t"
	  "movl 8(%esp), %eax\n\t"
	  "movl 20(%ecx), %edx\n\t"
	  "movl 0(%ecx), %ebx\n\t"
	  "movl 4(%ecx), %esi\n\t"
	  "movl 8(%ecx), %edi\n\t"
	  "movl 12(%ecx), %ebp\n\t"
	  "movl 16(%ecx), %esp\n\t"
	  "jmp *%edx\n");
#endif  /* CONFIG_FRAME_POINTER */
}

/*
 * I hope this is a correct interpretation...
 */
void abort(void)
{
  (*sys_exit)(-255);
}

/*
 * Just don't care for translations in the kernel.
 */
char * dcgettext(const char * domainname,
		 const char * msgid,
		 int category)
{
  return (char *)msgid;
}

