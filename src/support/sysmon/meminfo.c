// Copyright (C) 1992-1998 by Michael K. Johnson, johnsonm@redhat.com
// Copyright 1998-2003 Albert Cahalan
//
// This file is placed under the conditions of the GNU Library
// General Public License, version 2, or any later version.
// See file COPYING for information on distribution conditions.
//
// File for parsing top-level /proc entities. */
//
// June 2003, Fabian Frederick, disk and slab info

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

#include <unistd.h>
#include <fcntl.h>

#include "meminfo.h"


#define BAD_OPEN_MESSAGE					\
"Error: /proc must be mounted\n"				\
"  To mount /proc at boot you need an /etc/fstab line like:\n"	\
"      /proc   /proc   proc    defaults\n"			\
"  In the meantime, run \"mount /proc /proc -t proc\"\n"

#define MEMINFO_FILE "/proc/meminfo"

/* This macro opens filename only if necessary and seeks to 0 so
 * that successive calls to the functions are more efficient.
 * It also reads the current contents of the file into the global buf.
 */
#define FILE_TO_BUF(filename, fd) do{				\
    static int local_n;						\
    if (fd == -1 && (fd = open(filename, O_RDONLY)) == -1) {	\
	fputs(BAD_OPEN_MESSAGE, stderr);			\
	fflush(NULL);						\
	_exit(102);						\
    }								\
    lseek(fd, 0L, SEEK_SET);					\
    if ((local_n = read(fd, buf, sizeof buf - 1)) < 0) {	\
	perror(filename);					\
	fflush(NULL);						\
	_exit(103);						\
    }								\
    buf[local_n] = '\0';					\
    close(fd);							\
}while(0)

/* evals 'x' twice */
#define SET_IF_DESIRED(x,y) do{  if(x) *(x) = (y); }while(0)

/***********************************************************************/
/*
 * Copyright 1999 by Albert Cahalan; all rights reserved.
 * This file may be used subject to the terms and conditions of the
 * GNU Library General Public License Version 2, or any later version
 * at your option, as published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Library General Public License for more details.
 */

typedef struct mem_table_struct {
  const char *name;     /* memory type name */
  unsigned long *slot; /* slot in return struct */
} mem_table_struct;

static int compare_mem_table_structs(const void *a, const void *b){
  return strcmp(((const mem_table_struct*)a)->name,((const mem_table_struct*)b)->name);
}

/* example data, following junk, with comments added:
 *
 * MemTotal:        61768 kB    old
 * MemFree:          1436 kB    old
 * MemShared:           0 kB    old (now always zero; not calculated)
 * Buffers:          1312 kB    old
 * Cached:          20932 kB    old
 * Active:          12464 kB    new
 * Inact_dirty:      7772 kB    new
 * Inact_clean:      2008 kB    new
 * Inact_target:        0 kB    new
 * Inact_laundry:       0 kB    new, and might be missing too
 * HighTotal:           0 kB
 * HighFree:            0 kB
 * LowTotal:        61768 kB
 * LowFree:          1436 kB
 * SwapTotal:      122580 kB    old
 * SwapFree:        60352 kB    old
 * Inactive:        20420 kB    2.5.41+
 * Dirty:               0 kB    2.5.41+
 * Writeback:           0 kB    2.5.41+
 * Mapped:           9792 kB    2.5.41+
 * Slab:             4564 kB    2.5.41+
 * Committed_AS:     8440 kB    2.5.41+
 * PageTables:        304 kB    2.5.41+
 * ReverseMaps:      5738       2.5.41+
 * SwapCached:          0 kB    2.5.??+
 * HugePages_Total:   220       2.5.??+
 * HugePages_Free:    138       2.5.??+
 * Hugepagesize:     4096 kB    2.5.??+
 */


void meminfo(struct lwfs_meminfo *mi){
  int meminfo_fd = -1;
  char buf[1024];

  char namebuf[16]; /* big enough to hold any row name */
  mem_table_struct findme = { namebuf, NULL};
  mem_table_struct *found;
  char *head;
  char *tail;
  const mem_table_struct mem_table[] = {
  {"Active",       &mi->kb_active},       // important
  {"Buffers",      &mi->kb_main_buffers}, // important
  {"Cached",       &mi->kb_main_cached},  // important
  {"Committed_AS", &mi->kb_committed_as},
  {"Dirty",        &mi->kb_dirty},        // kB version of vmstat nr_dirty
  {"HighFree",     &mi->kb_high_free},
  {"HighTotal",    &mi->kb_high_total},
  {"Inact_clean",  &mi->kb_inact_clean},
  {"Inact_dirty",  &mi->kb_inact_dirty},
  {"Inact_laundry",&mi->kb_inact_laundry},
  {"Inact_target", &mi->kb_inact_target},
  {"Inactive",     &mi->kb_inactive},     // important
  {"LowFree",      &mi->kb_low_free},
  {"LowTotal",     &mi->kb_low_total},
  {"Mapped",       &mi->kb_mapped},       // kB version of vmstat nr_mapped
  {"MemFree",      &mi->kb_main_free},    // important
  {"MemShared",    &mi->kb_main_shared},  // important, but now gone!
  {"MemTotal",     &mi->kb_main_total},   // important
  {"PageTables",   &mi->kb_pagetables},   // kB version of vmstat nr_page_table_pages
  {"ReverseMaps",  &mi->nr_reversemaps},  // same as vmstat nr_page_table_pages
  {"Slab",         &mi->kb_slab},         // kB version of vmstat nr_slab
  {"SwapCached",   &mi->kb_swap_cached},
  {"SwapFree",     &mi->kb_swap_free},    // important
  {"SwapTotal",    &mi->kb_swap_total},   // important
  {"VmallocChunk", &mi->kb_vmalloc_chunk},
  {"VmallocTotal", &mi->kb_vmalloc_total},
  {"VmallocUsed",  &mi->kb_vmalloc_used},
  {"Writeback",    &mi->kb_writeback},    // kB version of vmstat nr_writeback
  };
  const int mem_table_count = sizeof(mem_table)/sizeof(mem_table_struct);

  FILE_TO_BUF(MEMINFO_FILE,meminfo_fd);

  mi->kb_inactive = ~0UL;

  head = buf;
  for(;;){
    tail = strchr(head, ':');
    if(!tail) break;
    *tail = '\0';
    if(strlen(head) >= sizeof(namebuf)){
      head = tail+1;
      goto nextline;
    }
    strcpy(namebuf,head);
    found = bsearch(&findme, mem_table, mem_table_count,
        sizeof(mem_table_struct), compare_mem_table_structs
    );
    head = tail+1;
    if(!found) goto nextline;
    *(found->slot) = strtoul(head,&tail,10);
nextline:
    tail = strchr(head, '\n');
    if(!tail) break;
    head = tail+1;
  }
  if(!mi->kb_low_total){  /* low==main except with large-memory support */
    mi->kb_low_total = mi->kb_main_total;
    mi->kb_low_free  = mi->kb_main_free;
  }
  if(mi->kb_inactive==~0UL){
    mi->kb_inactive = mi->kb_inact_dirty + mi->kb_inact_clean + mi->kb_inact_laundry;
  }
  mi->kb_swap_used = mi->kb_swap_total - mi->kb_swap_free;
  mi->kb_main_used = mi->kb_main_total - mi->kb_main_free;
}
