#ifndef MEMINFO_H_
#define MEMINFO_H_

struct lwfs_meminfo {
	/* obsolete */
	unsigned long kb_main_shared;
	/* old but still kicking -- the important stuff */
	unsigned long kb_main_buffers;
	unsigned long kb_main_cached;
	unsigned long kb_main_free;
	unsigned long kb_main_total;
	unsigned long kb_swap_free;
	unsigned long kb_swap_total;
	/* recently introduced */
	unsigned long kb_high_free;
	unsigned long kb_high_total;
	unsigned long kb_low_free;
	unsigned long kb_low_total;
	/* 2.4.xx era */
	unsigned long kb_active;
	unsigned long kb_inact_laundry;
	unsigned long kb_inact_dirty;
	unsigned long kb_inact_clean;
	unsigned long kb_inact_target;
	unsigned long kb_swap_cached;  /* late 2.4 and 2.6+ only */
	/* derived values */
	unsigned long kb_swap_used;
	unsigned long kb_main_used;
	/* 2.5.41+ */
	unsigned long kb_writeback;
	unsigned long kb_slab;
	unsigned long nr_reversemaps;
	unsigned long kb_committed_as;
	unsigned long kb_dirty;
	unsigned long kb_inactive;
	unsigned long kb_mapped;
	unsigned long kb_pagetables;
	// seen on a 2.6.x kernel:
	unsigned long kb_vmalloc_chunk;
	unsigned long kb_vmalloc_total;
	unsigned long kb_vmalloc_used;
};

void meminfo(struct lwfs_meminfo *mi);

#endif /*MEMINFO_H_*/
