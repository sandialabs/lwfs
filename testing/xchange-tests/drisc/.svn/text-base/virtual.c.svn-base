#include "config.h"
#include "drisc.h"
#include "drisc_internal.h"
#include "virtual.h"
#include "assert.h"
#ifndef LINUX_KERNEL_MODULE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <assert.h>
#else
#ifdef LINUX_KERNEL_MODULE
#ifndef __KERNEL__
#define __KERNEL__
#endif
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include "kdrisc.h"
#include "library.h"

#define printf	printk
#define fprintf(file, args...)	printk (args)
#define malloc (void *)DAllocMM
#define realloc(a,b) (void *)DReallocMM((addrs_t)a, b)
#define free(a) DFreeMM((addrs_t) a)
#define qsort _quicksort
#endif

extern char *arith3_name[];
extern char *arith2_name[];
extern char *drisc_type_names[];
extern char *branch_op_names[];

typedef void (*apply_func)(drisc_ctx c, basic_block b,
			   virtual_insn *ip, int loc);

static void apply_to_each(drisc_ctx c, void *insns, virtual_mach_info vmi,
			  apply_func func);

static const char *prefix_names[] = {"X86_CS_PREFIX", "X86_SS_PREFIX",
				     "X86_DS_PREFIX", "X86_ES_PREFIX",
				     "X86_FS_PREFIX", "X86_GS_PREFIX"};

#define OPND(x) ((x >= 100) ? ((type_of(c, x) == DR_B) ? 'B' : 'R') :'P'), x
extern int
virtual_print_insn(drisc_ctx c, void *info_ptr, void *i)
{
    virtual_insn *insn = (virtual_insn *)i;
    int insn_code = insn->insn_code;
    switch(insn->class_code) {
    case iclass_arith3:
        printf("%s %c%d, %c%d, %c%d", arith3_name[insn_code], 
	       OPND(insn->opnds.a3.dest),
	       OPND(insn->opnds.a3.src1), 
	       OPND(insn->opnds.a3.src2));
        break;
    case iclass_arith3i:
        printf("%si %c%d, %c%d, %ld", arith3_name[insn_code], 
	       OPND(insn->opnds.a3i.dest),
	       OPND(insn->opnds.a3i.src), insn->opnds.a3i.imm);
        break;
    case iclass_arith2:
        printf("%s %c%d, %c%d", arith2_name[insn_code],
	       OPND(insn->opnds.a2.dest), OPND(insn->opnds.a2.src));
        break;
    case iclass_convert:
    {
	int from_index = (insn->insn_code >> 4) & 0xf;
	int to_index = insn->insn_code & 0xf;
        printf("cv%s2%s %c%d, %c%d", drisc_type_names[from_index],
	       drisc_type_names[to_index], OPND(insn->opnds.a2.dest),
	       OPND(insn->opnds.a2.src));
        break;
    }
    case iclass_loadstore:
    {
	int typ = insn->insn_code & 0xf;
	int store = insn->insn_code >> 4;
        printf("%s%si %c%d, %c%d, %c%d", store == 0 ? "ld" : "st", 
	       drisc_type_names[typ], OPND(insn->opnds.a3.dest),
	       OPND(insn->opnds.a3.src1), OPND(insn->opnds.a3.src2));
        break;
    }
    case iclass_loadstorei:
    {
	int typ = insn->insn_code & 0xf;
	int store = insn->insn_code >> 4;
        printf("%s%si %c%d, %c%d, %ld", store == 0 ? "ld" : "st", 
	       drisc_type_names[typ], OPND(insn->opnds.a3i.dest),
	       OPND(insn->opnds.a3i.src), insn->opnds.a3i.imm);
        break;
    }
    case iclass_lea:
    {
        printf("lea %c%d, %c%d, %ld", OPND(insn->opnds.a3i.dest),
	       OPND(insn->opnds.a3i.src), insn->opnds.a3i.imm);
        break;
    }
    case iclass_set:
    {
	int typ = insn->insn_code & 0xf;
        printf("set%s %c%d, %ld", 
	       drisc_type_names[typ], OPND(insn->opnds.a3i.dest),
	       insn->opnds.a3i.imm);
        break;
    }
    case iclass_setf:
    {
	int typ = insn->insn_code & 0xf;
        printf("set%s %c%d, %g 0x(%lx)", 
	       drisc_type_names[typ], OPND(insn->opnds.sf.dest),
	       insn->opnds.sf.imm, *((long*)&insn->opnds.sf.imm));
        break;
    }
    case iclass_mov:
    {
	int typ = insn->insn_code & 0xf;
        printf("mov%s %c%d, %c%d", 
	       drisc_type_names[typ], OPND(insn->opnds.a2.dest),
	       OPND(insn->opnds.a2.src));
        break;
    }
    case iclass_reti:
    {
	int typ = insn->insn_code & 0xf;
        printf("ret%si %ld", 
	       drisc_type_names[typ], insn->opnds.a3i.imm);
        break;
    }
    case iclass_ret:
    {
	int typ = insn->insn_code & 0xf;
        printf("ret%s %c%d", 
	       drisc_type_names[typ], OPND(insn->opnds.a1.src));
        break;
    }
    case iclass_branch:
    {
	int br_op = insn->insn_code;
        printf("b%s %c%d, %c%d, L%d", branch_op_names[br_op], 
	       OPND(insn->opnds.br.src1), 
	       OPND(insn->opnds.br.src2), insn->opnds.br.label);
        break;
    }
    case iclass_branchi:
    {
	int br_op = insn->insn_code;
        printf("b%si %c%d, %ld, L%d", branch_op_names[br_op], 
	       OPND(insn->opnds.bri.src), 
	       insn->opnds.bri.imm, insn->opnds.bri.label);
        break;
    }
    case iclass_jump_to_label:
    {
        printf("br L%d", insn->opnds.br.label);
        break;
    }
    case iclass_jump_to_reg:
    {
        printf("jmp %c%d", OPND(insn->opnds.br.src1));
	break;
    }
    case iclass_jump_to_imm:
    {
        printf("jmp 0x%lx", insn->opnds.bri.imm);
        break;
    }
    case iclass_special:
    {
	switch (insn->opnds.spec.type) {
	case DR_NOP:
	    printf("special NOP");
	    break;
	case DR_SEGMENTED_FOLLOWS:
	    printf("special SEGMENTED %s", 
		   prefix_names[insn->opnds.spec.param]);
	    break;
	}
        break;
    }
    case iclass_call:
    {
	int typ = insn->insn_code & 0xf;
	int reg = insn->insn_code & 0x10;
	if (typ != DR_V) {
	    if (reg != 0) {
		printf("call%s R%ld, %c%d", drisc_type_names[typ], 
		       insn->opnds.bri.imm, OPND(insn->opnds.bri.src));
	    } else {
		printf("call%s 0x%lx, %c%d", drisc_type_names[typ], 
		       insn->opnds.bri.imm, OPND(insn->opnds.bri.src));
	    }
	} else {
	    if (reg != 0) {
		printf("call%s R%ld", drisc_type_names[typ], 
		       insn->opnds.bri.imm);
	    } else {
		printf("call%s 0x%lx", drisc_type_names[typ], 
		       insn->opnds.bri.imm);
	    }
	}
        break;
    }
    case iclass_push:
    {
	int typ = insn->insn_code & 0xf;
	if (insn->opnds.a1.src == 0xffff) {
	    printf("pushinit");
	} else {
	    printf("push%s %c%d", drisc_type_names[typ], 
		   OPND(insn->opnds.a1.src));
	}
        break;
    }
    case iclass_pushi:
    {
	int typ = insn->insn_code & 0xf;
        printf("push%si 0x%lx", drisc_type_names[typ], insn->opnds.a3i.imm);
        break;
    }
    case iclass_pushf:
    {
	int typ = insn->insn_code;
        printf("push%si %g", drisc_type_names[typ], insn->opnds.sf.imm);
        break;
    }
    case iclass_nop:
	printf("nop");
    }
    return sizeof(*insn);
}

static int
insn_same_except_dest(virtual_insn *i, virtual_insn *j)
{
    int icode = i->insn_code;
    int jcode = j->insn_code;
    
    if (i->class_code != j->class_code) return 0;
    switch(i->class_code) {
    case iclass_arith3:
	return ((icode == jcode) && (i->opnds.a3.src1 == j->opnds.a3.src1) &&
		(i->opnds.a3.src2 == j->opnds.a3.src2));
        break;
    case iclass_arith3i:
	return ((icode == jcode) && (i->opnds.a3i.src == j->opnds.a3i.src) &&
		(i->opnds.a3i.imm == j->opnds.a3i.imm));
        break;
    case iclass_arith2:
	return ((icode == jcode) && (i->opnds.a2.src == j->opnds.a2.src));
        break;
    case iclass_convert:
	return ((icode == jcode) && (i->opnds.a2.src == j->opnds.a2.src));
	break;
    case iclass_loadstore:
	return ((icode == jcode) && (i->opnds.a3.src1 == j->opnds.a3.src1) &&
		(i->opnds.a3.src2 == j->opnds.a3.src2));
    case iclass_loadstorei:
	return ((icode == jcode) && (i->opnds.a3i.src == j->opnds.a3i.src) &&
		(i->opnds.a3i.imm == j->opnds.a3i.imm));
    case iclass_lea:
	return ((i->opnds.a3i.src == j->opnds.a3i.src) &&
		(i->opnds.a3i.imm == j->opnds.a3i.imm));
    case iclass_set:
    case iclass_setf:
	return 0;
    case iclass_mov:
	return ((icode == jcode) && (i->opnds.a2.src == j->opnds.a2.src));
    case iclass_reti:
    case iclass_ret:
    case iclass_branch:
    case iclass_branchi:
    case iclass_jump_to_label:
    case iclass_jump_to_reg:
    case iclass_jump_to_imm:
    case iclass_special:
    case iclass_call:
    case iclass_push:
    case iclass_pushi:
    case iclass_pushf:
    case iclass_nop:
	break;
    }
    return 0;
}

static void
pushpop_inuse_regs(drisc_ctx c, int pop, virtual_insn *ip)
{
    int i = 0;
    /* no actual reg assigns, so only the params are in use */
    for(i=0; i<c->p->c_param_count; i++) {
	if (c->p->c_param_args[i].is_register) {
	    c->j->save_restore(c, pop, c->p->c_param_args[i].type,
			       c->p->c_param_args[i].in_reg);
	}
    }
}


#define end_bb(b, lab, f) \
bb->end_branch_label = lab;\
bb->fall_through = f;\
bb->end = i - 1;\
vmi->bbcount++;\
vmi->bblist = realloc(vmi->bblist, sizeof(struct basic_block) * (vmi->bbcount +1));\
bb = &vmi->bblist[vmi->bbcount];\
bb->start = i;\
bb->label = -1;\
bb->regs_used = new_bit_vec(c->p->vreg_count);\
bb->regs_defined = new_bit_vec(c->p->vreg_count);

static bit_vec
new_bit_vec(int max)
{
    int len = (max +7) >> 3;
    bit_vec ret = malloc(sizeof(struct bitv) + len - 2);
    ret->len = len;
    memset(&ret->vec[0], 0, len);
    return ret;
}

static void
init_bit_vec(bit_vec vec, int size)
{
    int len = size - 2;
    vec->len = len;
    memset(&vec->vec[0], 0, len);
}

static void
bit_vec_set(bit_vec v, int bit)
{
    int index = bit >> 3;
    int mask = 1 << (bit & 0x7);
    v->vec[index] |= mask;
}

static void
bit_vec_clear(bit_vec v, int bit)
{
    int index = bit >> 3;
    int mask = ~(1 << (bit & 0x7));
    v->vec[index] &= mask;
}

static int
bit_vec_is_set(bit_vec v, int bit)
{
    int index = bit >> 3;
    int mask = 1 << (bit & 0x7);
    return ((v->vec[index] & mask) == mask);
}

static void
dump_reg_vec(bit_vec v)
{
    int i;
    if (v == NULL) return;
    for (i=0; i < v->len; i++) {
	int j;
	for (j=0; j < 7; j++) {
	    if ((v->vec[i] & (1 << j)) != 0) {
		printf(" R%d", i * 8 + j + 100);
	    }
	}
    }
}

static int
has_single_def_use(drisc_ctx c, int vreg)
{
    if (vreg >= 100) {
	return ((c->p->vregs[vreg-100].use_info.use_count == 1) &&
		(c->p->vregs[vreg-100].use_info.def_count == 1));
    }
    return 0;
}

static void
set_unused(drisc_ctx c, basic_block bb, int vreg)
{
    if (vreg >= 100) {
	c->p->vregs[vreg-100].use_info.use_count = 0;
	c->p->vregs[vreg-100].use_info.def_count = 0;
	bit_vec_clear(bb->regs_defined, vreg - 100);
	bit_vec_clear(bb->regs_used, vreg - 100);
    }	
}

static void
set_used(drisc_ctx c, int vreg)
{
    if (vreg >= 100) {
	c->p->vregs[vreg-100].use_info.use_count++;
    }
}

static void
set_defined(drisc_ctx c, int vreg)
{
    if (vreg >= 100) {
	c->p->vregs[vreg-100].use_info.def_count++;
    }
}

static void bb_defines(drisc_ctx c, basic_block bb, int vreg)
{
    if (vreg >= 100) {
	/* not param */
	if (!bit_vec_is_set(bb->regs_used, vreg - 100)) {
	    bit_vec_set(bb->regs_defined, vreg - 100);
	}
	set_defined(c, vreg);
    }
}

static void bb_uses(drisc_ctx c, basic_block bb, int vreg)
{
    if (vreg >= 100) {
	/* not param */
	if (!bit_vec_is_set(bb->regs_defined, vreg - 100)) {
	    bit_vec_set(bb->regs_used, vreg - 100);
	}
	set_used(c, vreg);
    }
}

static void
add_pred(basic_block bb, int pred)
{
    bb->pred_list = realloc(bb->pred_list, (bb->pred_count+1) * sizeof(int));
    bb->pred_list[bb->pred_count] = pred;
    bb->pred_count++;
}

static void
dump_bbs(drisc_ctx c)
{
    virtual_mach_info vmi = (virtual_mach_info)c->p->mach_info;
    int i, j;
    for (i=0; i < vmi->bbcount; i++) {
	printf("\nBasic block %d, start %d, end %d, label %d, fall %d, branch_to %d\n",
	       i, vmi->bblist[i].start, vmi->bblist[i].end, 
	       vmi->bblist[i].label, vmi->bblist[i].fall_through,
	       vmi->bblist[i].end_branch_label);
	printf("    defines :"); dump_reg_vec(vmi->bblist[i].regs_defined);
	printf("\n    uses :"); dump_reg_vec(vmi->bblist[i].regs_used);
	printf("\n    live_at_end :"); dump_reg_vec(vmi->bblist[i].live_at_end);
	printf("\n    succ :"); 
	for (j=0; j < vmi->bblist[i].succ_count; j++) {
	    printf(" %d", vmi->bblist[i].succ_list[j]);
	}
	printf("\n    preds :"); 
	for (j=0; j < vmi->bblist[i].pred_count; j++) {
	    printf(" %d", vmi->bblist[i].pred_list[j]);
	}
	if (vmi->bblist[i].is_loop_start) printf(" - LOOP_START");
	if (vmi->bblist[i].is_loop_end) printf(" - LOOP_END");
/*NOTUSED	printf("   Nesting depth %d", vmi->bblist[i].loop_depth);*/
	printf("\n");
	for (j = vmi->bblist[i].start; j <= vmi->bblist[i].end; j++) {
	    printf(" %d - ", j);
	    virtual_print_insn(c, NULL, ((char *)c->p->code_base) + 
			       j * sizeof(virtual_insn));
	    printf("\n");
	}
    }
}

static int
add_regs(bit_vec dest, bit_vec src)
{
    int change = 0;
    int i, len = dest->len;
    if (len > src->len) i = src->len;
    for (i=0; i < len; i++) {
	char tmp = (dest->vec[i] | src->vec[i]);
	if (tmp != dest->vec[i]) {
	    change = 1;
	    dest->vec[i] = tmp;
	}
    }
    return change;
}

static void
remove_regs(bit_vec dest, bit_vec src)
{
    int i, len = dest->len;
    if (len > src->len) i = src->len;
    for (i=0; i < len; i++) {
	dest->vec[i] = (dest->vec[i] & ~src->vec[i]);
    }
}

static void
clear_bit_vec(bit_vec b)
{
    memset(&b->vec[0], 0, b->len);
}

static void
do_depth(drisc_ctx c, int *pred_array, int count)
{
    virtual_mach_info vmi = (virtual_mach_info)c->p->mach_info;
    basic_block bb = &vmi->bblist[pred_array[count]];
    int i;

    if (bb->visited) return;
    bb->visited++;
    for (i = 0; i < bb->succ_count; i++) {
	int succ = bb->succ_list[i];
	int j;
	for (j = 0; j <= count; j++) {
	    if (succ == pred_array[j]) {
		/* found a loop */
		if (c->drisc_debug) printf("bb %d has succ %d, which appears as entry %d in the pred list\n", pred_array[count], succ, j);
		bb->is_loop_end++;
		vmi->bblist[succ].is_loop_start++;
		break;
	    }
	}
	pred_array[count + 1] = succ;
	do_depth(c, pred_array, count + 1);
    }
}
	
static void
mark_depth(drisc_ctx c, int *pred_array, int count, int depth)
{
    virtual_mach_info vmi = (virtual_mach_info)c->p->mach_info;
    basic_block bb = &vmi->bblist[pred_array[count]];
    int i, succ_depth;

    if (c->drisc_debug) 
	printf("Considering bb %d, loop_end_state %d, loop_start %d, in depth %d\n", pred_array[count], bb->is_loop_end, bb->is_loop_start, depth);
    if (bb->visited) return;
    bb->visited++;
    bb->loop_depth = depth;

    if (bb->is_loop_start) bb->loop_depth++;

    succ_depth = bb->loop_depth;

    if (bb->is_loop_end) succ_depth--;

    for (i = 0; i < bb->succ_count; i++) {
	int succ = bb->succ_list[i];
	pred_array[count + 1] = succ;
	mark_depth(c, pred_array, count + 1, succ_depth);
    }
}
	

#ifdef NOT_USED
static void
calculate_depth(drisc_ctx c)
{
    virtual_mach_info vmi = (virtual_mach_info)c->p->mach_info;
    int *pred_array = malloc(sizeof(pred_array[0]) *vmi->bbcount);
    int pred_count = 0;
    int i = 0;
    for (i=0; i < vmi->bbcount; i++) {
	vmi->bblist[i].visited = 0;
	vmi->bblist[i].loop_depth = 0;
	vmi->bblist[i].is_loop_start = 0;
	vmi->bblist[i].is_loop_end = 0;
    }
    pred_array[0] = 0;
    do_depth(c, pred_array, pred_count);
    for (i=0; i < vmi->bbcount; i++) vmi->bblist[i].visited = 0;
    mark_depth(c, pred_array, pred_count, 0);
}
#endif
    
static void
build_live(drisc_ctx c)
{
    virtual_mach_info vmi = (virtual_mach_info)c->p->mach_info;
    int i, change = 1;
    bit_vec live_at_begin = new_bit_vec(c->p->vreg_count);
    for (i=0; i < vmi->bbcount; i++) {
	basic_block bb = &vmi->bblist[i];
	bb->live_at_end = new_bit_vec(c->p->vreg_count);
    }
    for (i= 0; i < vmi->bbcount; i++) {
	basic_block bb = &vmi->bblist[i];
	int j;
	for (j = 0 ; j < bb->pred_count; j++) {
	    /* foreach predecessor add regs we use to preds live_at_end */
	    basic_block pred_bb = &vmi->bblist[bb->pred_list[j]];
	    (void) add_regs(pred_bb->live_at_end, bb->regs_used);
	}
    }
    while (change) {
	/* go through the bbs until live lists stabilize */
	change = 0;
	for (i= 0; i < vmi->bbcount; i++) {
	    basic_block bb = &vmi->bblist[i];
	    int j;

	    clear_bit_vec(live_at_begin);
	    add_regs(live_at_begin, bb->live_at_end);
	    remove_regs(live_at_begin, bb->regs_defined);
	    for (j = 0 ; j < bb->pred_count; j++) {
		/* 
		 * foreach predecessor add regs live at our beginning 
		 * to preds live_at_end 
		 */
		basic_block pred_bb = &vmi->bblist[bb->pred_list[j]];
		change |= add_regs(pred_bb->live_at_end, live_at_begin);
	    }
	}
    }
    free(live_at_begin);
}

static int
insn_defines(virtual_insn *insn)
{
    switch(insn->class_code) {
    case iclass_arith3:
	return insn->opnds.a3.dest;
    case iclass_arith3i:
	return insn->opnds.a3i.dest;
    case iclass_arith2:
	return insn->opnds.a2.dest;
    case iclass_convert:
	return insn->opnds.a2.dest;
    case iclass_loadstorei:
	if (insn->insn_code >> 4) {
	    /* store */
	    return -1;
	} else {
	    return insn->opnds.a3i.dest;
	}
    case iclass_lea:
	return insn->opnds.a3i.dest;
    case iclass_loadstore:
	if (insn->insn_code >> 4) {
	    /* store */
	    return -1;
	} else {
	    return insn->opnds.a3.dest;
	}
    case iclass_set:
	return insn->opnds.a3i.dest;
    case iclass_setf:
	return insn->opnds.sf.dest;
    case iclass_mov:
	return insn->opnds.a2.dest;
    case iclass_reti:
    case iclass_ret:
    case iclass_branch:
    case iclass_branchi:
    case iclass_jump_to_label:
    case iclass_jump_to_reg:
    case iclass_jump_to_imm:
    case iclass_special:
	return -1;
    case iclass_call:
    {
	int typ = insn->insn_code & 0xf;
	/* put the return register definition in the next bb */
	if (typ != DR_V) {
	    return insn->opnds.bri.src;
	}
	return -1;
    }
    case iclass_push:
    case iclass_pushi:
    case iclass_pushf:
    case iclass_nop:
	return -1;
    default:
	assert(0);
    }
    return -1;
}

static int
insn_define_test(virtual_insn *insn, int vreg)
{
    switch(insn->class_code) {
    case iclass_arith3:
	return insn->opnds.a3.dest == vreg;
    case iclass_arith3i:
	return insn->opnds.a3i.dest == vreg;
    case iclass_arith2:
	return insn->opnds.a2.dest == vreg;
    case iclass_convert:
	return insn->opnds.a2.dest == vreg;
    case iclass_lea:
	return insn->opnds.a3i.dest == vreg;
    case iclass_loadstorei:
	if (insn->insn_code >> 4) {
	    /* store */
	    return 0;
	} else {
	    return insn->opnds.a3i.dest == vreg;
	}
    case iclass_loadstore:
	if (insn->insn_code >> 4) {
	    /* store */
	    return 0;
	} else {
	    return insn->opnds.a3.dest == vreg;
	}
    case iclass_set:
	return insn->opnds.a3i.dest == vreg;
    case iclass_setf:
	return insn->opnds.sf.dest == vreg;
    case iclass_mov:
	return insn->opnds.a2.dest == vreg;
    case iclass_reti:
    case iclass_ret:
    case iclass_branch:
    case iclass_branchi:
    case iclass_jump_to_label:
    case iclass_jump_to_reg:
    case iclass_jump_to_imm:
    case iclass_special:
	return 0;
    case iclass_call:
    {
	int typ = insn->insn_code & 0xf;
	/* put the return register definition in the next bb */
	if (typ != DR_V) {
	    return insn->opnds.bri.src == vreg;
	}
	return 0;
    }
    case iclass_push:
    case iclass_pushi:
    case iclass_pushf:
    case iclass_nop:
	return 0;
    default:
	assert(0);
    }
    return 0;
}


static void
insn_uses(virtual_insn *insn, int *used)
{
    used[0] = -1;
    used[1] = -1;
    used[2] = -1;
    switch(insn->class_code) {
    case iclass_arith3:
	used[0] = insn->opnds.a3.src1;
	used[1] = insn->opnds.a3.src2;
	break;
    case iclass_arith3i:
	used[0] = insn->opnds.a3i.src;
	break;
    case iclass_arith2:
	used[0] = insn->opnds.a2.src;
	break;
    case iclass_convert:
	used[0] = insn->opnds.a2.src;
	break;
    case iclass_lea:
	used[0] = insn->opnds.a3i.src;
	break;
    case iclass_loadstorei:
	used[0] = insn->opnds.a3i.src;
	if (insn->insn_code >> 4) {
	    /* store */
	    used[1] = insn->opnds.a3i.dest;
	}
	break;
    case iclass_loadstore:
	used[0] = insn->opnds.a3.src1;
	used[1] = insn->opnds.a3.src2;
	if (insn->insn_code >> 4) {
	    /* store */
	    used[2] = insn->opnds.a3.dest;
	}
	break;
    case iclass_set:
	break;
    case iclass_setf:
	break;
    case iclass_mov:
	used[0] = insn->opnds.a2.src;
	break;
    case iclass_reti:
	break;
    case iclass_ret:
	used[0] = insn->opnds.a1.src;
	break;
    case iclass_branch:
	used[0] = insn->opnds.br.src1;
	used[1] = insn->opnds.br.src2;
	break;
    case iclass_branchi:
	used[0] = insn->opnds.bri.src;
	break;
    case iclass_jump_to_label:
	break;
    case iclass_jump_to_reg:
	used[0] = insn->opnds.bri.src;
	break;
    case iclass_jump_to_imm:
    case iclass_special:
	break;
    case iclass_call: {
	int reg = insn->insn_code & 0x10;
	if (reg != 0) {
	    long imm = insn->opnds.bri.imm;
	    int src1_vreg = imm;
	    used[0] = src1_vreg;
	}
	break;
    }
    case iclass_push:
	if (insn->opnds.a1.src != 0xffff) 
	    used[0] = insn->opnds.a1.src;
	break;
    case iclass_pushi:
	break;
    case iclass_pushf:
	break;
    case iclass_nop:
	break;
    }
}

static void
replace_insn_src(virtual_insn *insn, int replace_vreg, int new_vreg)
{
    switch(insn->class_code) {
    case iclass_arith3:
	if (replace_vreg == insn->opnds.a3.src1) {
	    insn->opnds.a3.src1 = new_vreg; return;};
	if (replace_vreg == insn->opnds.a3.src2) {
	    insn->opnds.a3.src2 = new_vreg; return;};
	break;
    case iclass_arith3i:
	if (replace_vreg == insn->opnds.a3i.src) {
	    insn->opnds.a3i.src = new_vreg; return;};
	break;
    case iclass_arith2:
	if (replace_vreg == insn->opnds.a2.src) {
	    insn->opnds.a2.src = new_vreg; return;};
	break;
    case iclass_convert:
	if (replace_vreg == insn->opnds.a2.src) {
	    insn->opnds.a2.src = new_vreg; return;};
	break;
    case iclass_lea:
	if (replace_vreg == insn->opnds.a3i.src) {
	    insn->opnds.a3i.src = new_vreg; return;};
	break;
    case iclass_loadstorei:
	if (replace_vreg == insn->opnds.a3i.src) {
		insn->opnds.a3i.src = new_vreg; return;};
	if (insn->insn_code >> 4) {
	    /* store */
	    if (replace_vreg == insn->opnds.a3i.dest) {
		insn->opnds.a3i.dest = new_vreg; return;};
	}
	break;
    case iclass_loadstore:
	if (replace_vreg == insn->opnds.a3.src1) {
		insn->opnds.a3.src1 = new_vreg; return;};
	if (replace_vreg == insn->opnds.a3.src2) {
		insn->opnds.a3.src2 = new_vreg; return;};
	if (insn->insn_code >> 4) {
	    /* store */
	    if (replace_vreg == insn->opnds.a3.dest) {
		insn->opnds.a3.dest = new_vreg; return;};
	}
	break;
    case iclass_set:
	break;
    case iclass_setf:
	break;
    case iclass_mov:
	if (replace_vreg == insn->opnds.a2.src) {
	    insn->opnds.a2.src = new_vreg; return;};
	break;
    case iclass_reti:
	break;
    case iclass_ret:
	if (replace_vreg == insn->opnds.a1.src) {
	    insn->opnds.a1.src = new_vreg; return;};
	break;
    case iclass_branch:
	if (replace_vreg == insn->opnds.br.src1) {
	    insn->opnds.br.src1 = new_vreg; return;};
	if (replace_vreg == insn->opnds.br.src2) {
	    insn->opnds.br.src2 = new_vreg; return;};
	break;
    case iclass_branchi:
	if (replace_vreg == insn->opnds.bri.src) {
	    insn->opnds.bri.src = new_vreg; return;};
	break;
    case iclass_jump_to_label:
	break;
    case iclass_jump_to_reg:
	if (replace_vreg == insn->opnds.bri.src) {
	    insn->opnds.bri.src = new_vreg; return;};
	break;
    case iclass_jump_to_imm:
    case iclass_special:
	break;
    case iclass_call:
	break;
    case iclass_push:
	if (insn->opnds.a1.src != 0xffff) 
	    if (replace_vreg == insn->opnds.a1.src) {
		insn->opnds.a1.src = new_vreg; return;};
	break;
    case iclass_pushi:
	break;
    case iclass_pushf:
	break;
    case iclass_nop:
	break;
    }
    assert(0);
}

static int
insn_use_test(virtual_insn *insn, int vreg)
{
    switch(insn->class_code) {
    case iclass_arith3:
	if (vreg == insn->opnds.a3.src1) return 1;
	if (vreg == insn->opnds.a3.src2) return 1;
	break;
    case iclass_arith3i:
	if (vreg == insn->opnds.a3i.src) return 1;
	break;
    case iclass_arith2:
	if (vreg == insn->opnds.a2.src) return 1;
	break;
    case iclass_convert:
	if (vreg == insn->opnds.a2.src) return 1;
	break;
    case iclass_lea:
	if (vreg == insn->opnds.a3i.src) return 1;
	break;
    case iclass_loadstorei:
	if (vreg == insn->opnds.a3i.src) return 1;
	if (insn->insn_code >> 4) {
	    /* store */
	    if (vreg == insn->opnds.a3i.dest) return 1;
	}
	break;
    case iclass_loadstore:
	if (vreg == insn->opnds.a3.src1) return 1;
	if (vreg == insn->opnds.a3.src2) return 1;
	if (insn->insn_code >> 4) {
	    /* store */
	    if (vreg == insn->opnds.a3.dest) return 1;
	}
	break;
    case iclass_set:
	break;
    case iclass_setf:
	break;
    case iclass_mov:
	if (vreg == insn->opnds.a2.src) return 1;
	break;
    case iclass_reti:
	break;
    case iclass_ret:
	if (vreg == insn->opnds.a1.src) return 1;
	break;
    case iclass_branch:
	if (vreg == insn->opnds.br.src1) return 1;
	if (vreg == insn->opnds.br.src2) return 1;
	break;
    case iclass_branchi:
	if (vreg == insn->opnds.bri.src) return 1;
	break;
    case iclass_jump_to_label:
	break;
    case iclass_jump_to_reg:
	if (vreg == insn->opnds.bri.src) return 1;
	break;
    case iclass_jump_to_imm:
    case iclass_special:
	break;
    case iclass_call:{
	int reg = insn->insn_code & 0x10;
	if (reg != 0) {
	    long imm = insn->opnds.bri.imm;
	    int src1_vreg = imm;
	    if (vreg == src1_vreg) return 1;
	}
	break;
    }
    case iclass_push:
	if (insn->opnds.a1.src != 0xffff) 
	    if (vreg == insn->opnds.a1.src) return 1;
	break;
    case iclass_pushi:
	break;
    case iclass_pushf:
	break;
    case iclass_nop:
	break;
    }
    return 0;
}

static void
free_bbs(virtual_mach_info vmi)
{
    int i;
    for (i=0 ; i < vmi->bbcount; i++) {
	basic_block bb = &vmi->bblist[i];
	if (bb->pred_list) free(bb->pred_list);
	if (bb->succ_list) free(bb->succ_list);
	free(bb->regs_used);
	free(bb->regs_defined);
	free(bb->live_at_end);
	if (bb->reg_assigns) free(bb->reg_assigns);
    }
    if (vmi->bblist) free(vmi->bblist);
    vmi->bblist = NULL;
    vmi->bbcount = 0;
}

static void
build_bbs(drisc_ctx c, void *vinsns, void *code_end)
{
    virtual_mach_info vmi = (virtual_mach_info)c->p->mach_info;
    basic_block bb;
    int i, j;
    virtual_insn *insn, *insns = vinsns;
    struct branch_table *t = &c->p->branch_table;

    vmi->bbcount = 0;
    vmi->bblist = malloc(sizeof(struct basic_block));
    i = 0;
    bb = vmi->bblist;
    bb->start = 0;
    bb->label = -1;
    bb->regs_used = new_bit_vec(c->p->vreg_count);
    bb->regs_defined = new_bit_vec(c->p->vreg_count);
    bb->reg_assigns = NULL;
    while((insn = &insns[i++]) < (virtual_insn *)code_end) {
	switch(insn->class_code) {
	case iclass_arith3:
	    bb_uses(c, bb, insn->opnds.a3.src1);
	    bb_uses(c, bb, insn->opnds.a3.src2);
	    bb_defines(c, bb,  insn->opnds.a3.dest);
	    break;
	case iclass_arith3i:
	    bb_uses(c, bb, insn->opnds.a3i.src);
	    bb_defines(c, bb, insn->opnds.a3i.dest);
	    break;
	case iclass_arith2:
	    bb_uses(c, bb, insn->opnds.a2.src);
	    bb_defines(c, bb, insn->opnds.a2.dest);
	    break;
	case iclass_convert:
	    bb_uses(c, bb, insn->opnds.a2.src);
	    bb_defines(c, bb, insn->opnds.a2.dest);
	    break;
	case iclass_lea:
	    bb_uses(c, bb, insn->opnds.a3i.src);
	    bb_defines(c, bb, insn->opnds.a3i.dest);
	    break;
	case iclass_loadstorei:
	    bb_uses(c, bb, insn->opnds.a3i.src);
	    if (insn->insn_code >> 4) {
		/* store */
		bb_uses(c, bb, insn->opnds.a3i.dest);
	    } else {
		bb_defines(c, bb, insn->opnds.a3i.dest);
	    }
	    break;
	case iclass_loadstore:
	    bb_uses(c, bb, insn->opnds.a3.src1);
	    bb_uses(c, bb, insn->opnds.a3.src2);
	    if (insn->insn_code >> 4) {
		/* store */
		bb_uses(c, bb, insn->opnds.a3.dest);
	    } else {
		bb_defines(c, bb, insn->opnds.a3.dest);
	    }
	    break;
	case iclass_set:
	    bb_defines(c, bb, insn->opnds.a3i.dest);
	    break;
	case iclass_setf:
	    bb_defines(c, bb, insn->opnds.sf.dest);
	    break;
	case iclass_mov:
	    bb_uses(c, bb, insn->opnds.a2.src);
	    bb_defines(c, bb, insn->opnds.a2.dest);
	    break;
	case iclass_reti:
	    break;
	case iclass_ret:
	    bb_uses(c, bb, insn->opnds.a1.src);
	    break;
	case iclass_branch:
	    bb_uses(c, bb, insn->opnds.br.src1);
	    bb_uses(c, bb, insn->opnds.br.src2);
	    end_bb(bb, insn->opnds.br.label, 1);
	    break;
	case iclass_branchi:
	    bb_uses(c, bb, insn->opnds.bri.src);
	    end_bb(bb, insn->opnds.bri.label, 1);
	    break;
	case iclass_jump_to_label:
	    end_bb(bb, insn->opnds.br.label, 0);
	    break;
	case iclass_jump_to_reg:
	    bb_uses(c, bb, insn->opnds.br.src1);
	    end_bb(bb, -1, 0);
	    break;
	case iclass_jump_to_imm:
	    end_bb(bb, -1, 0);
	    break;
	case iclass_call:
	{
	    int typ = insn->insn_code & 0xf;
	    end_bb(bb, -1, 1);
	    /* put the return register definition in the next bb */
	    if (typ != DR_V) {
		bb_defines(c, bb, insn->opnds.bri.src);
	    }
	    break;
	}
	case iclass_push:
	    if (insn->opnds.a1.src != 0xffff) 
		bb_uses(c, bb, insn->opnds.a1.src);
	    break;
	case iclass_special:
	    break;
	case iclass_pushi:
	    break;
	case iclass_pushf:
	    break;
	case iclass_nop:
	    break;
	}
	for (j=0; j < t->next_label; j++) {
	    if ((unsigned)t->label_locs[j] == 
		((char*)insn - (char*)insns) + sizeof(virtual_insn)) {
		if (bb->start != i) {
		    end_bb(bb, -1, 1);
		}
		bb->label = j;
	    }
	}
    }
    end_bb(bb, -1, 0);
    free(bb->regs_used);
    free(bb->regs_defined);
    (bb - 1) ->end--;
    for (i=0; i < vmi->bbcount; i++) {
	basic_block bb = &vmi->bblist[i];
	bb->pred_count = bb->succ_count = 0;
	bb->succ_list = malloc(2* sizeof(int));
	bb->pred_list = malloc(sizeof(int));
	bb->reg_assigns = NULL;
    }
    for (i=0; i < vmi->bbcount; i++) {
	basic_block bb = &vmi->bblist[i];
	if (bb->fall_through) {
	    bb->succ_list[bb->succ_count] = i+1;
	    bb->succ_count++;
	    add_pred(&vmi->bblist[i+1], i);
	}
	if (bb->end_branch_label != -1) {
	    int j;
	    for (j=0; j < vmi->bbcount; j++) {
		if (vmi->bblist[j].label == bb->end_branch_label) {
		    bb->succ_list[bb->succ_count] = j;
		    bb->succ_count++;
		    add_pred(&vmi->bblist[j], i);
		    j = vmi->bbcount;
		}
	    }
	}
    }
    build_live(c);
/*    calculate_depth(c);*/
}


typedef void (*bv_func)(int bit, void *p1, void *p2);

static void
do_reg_assign(int bit, basic_block bb, drisc_ctx c)
{
    /* no local preg for live registers */
    int preg;
    if ((c->p->vregs[bit].use_info.use_count == 0) &&
	(c->p->vregs[bit].use_info.def_count == 0)) {
	if (c->drisc_debug)
	    printf("virtual reg %d optimized away\n", bit + 100);
	return;
    }
    if (bit_vec_is_set(bb->live_at_end, bit)) {
	c->p->vregs[bit].need_assign = 1;
	return;
    }
    if (dr_getreg(c, &preg, c->p->vregs[bit].typ, DR_VAR) == 0) {
	c->p->vregs[bit].need_assign = 1;
	bb->reg_assigns[bit] = -1;
	if (c->drisc_debug)
	    printf("No more tmp regs for virtual reg %d\n", bit + 100);
    } else {
	bb->reg_assigns[bit] = preg;
	if (c->drisc_debug)
	    printf("virtual reg %d assigned to preg %d\n",
		   bit + 100, bb->reg_assigns[bit]);
    }
}

static void
do_use_def_count(drisc_ctx c, basic_block bb, virtual_insn *insns, int loc)
{
    virtual_insn *insn = &((virtual_insn *)insns)[loc];
    switch(insn->class_code) {
    case iclass_arith3:
	set_used(c, insn->opnds.a3.src1);
	set_used(c, insn->opnds.a3.src2);
	set_defined(c,  insn->opnds.a3.dest);
	break;
    case iclass_arith3i:
	set_used(c, insn->opnds.a3i.src);
	set_defined(c, insn->opnds.a3i.dest);
	break;
    case iclass_arith2:
	set_used(c, insn->opnds.a2.src);
	set_defined(c, insn->opnds.a2.dest);
	break;
    case iclass_convert:
	set_used(c, insn->opnds.a2.src);
	set_defined(c, insn->opnds.a2.dest);
	break;
    case iclass_lea:
	set_used(c, insn->opnds.a3i.src);
	set_defined(c, insn->opnds.a3i.dest);
	break;
    case iclass_loadstorei:
	set_used(c, insn->opnds.a3i.src);
	if (insn->insn_code >> 4) {
	    /* store */
	    set_used(c, insn->opnds.a3i.dest);
	} else {
	    set_defined(c, insn->opnds.a3i.dest);
	}
	break;
    case iclass_loadstore:
	set_used(c, insn->opnds.a3.src1);
	set_used(c, insn->opnds.a3.src2);
	if (insn->insn_code >> 4) {
	    /* store */
	    set_used(c, insn->opnds.a3.dest);
	} else {
	    set_defined(c, insn->opnds.a3.dest);
	}
	break;
    case iclass_set:
	set_defined(c, insn->opnds.a3i.dest);
	break;
    case iclass_setf:
	set_defined(c, insn->opnds.sf.dest);
	break;
    case iclass_mov:
	set_used(c, insn->opnds.a2.src);
	set_defined(c, insn->opnds.a2.dest);
	break;
    case iclass_reti:
	break;
    case iclass_ret:
	set_used(c, insn->opnds.a1.src);
	break;
    case iclass_branch:
	set_used(c, insn->opnds.br.src1);
	set_used(c, insn->opnds.br.src2);
	break;
    case iclass_branchi:
	set_used(c, insn->opnds.bri.src);
	break;
    case iclass_jump_to_label:
	break;
    case iclass_jump_to_reg:
	set_used(c, insn->opnds.br.src1);
	break;
    case iclass_jump_to_imm:
	break;
    case iclass_special:
	break;
    case iclass_call:
	if ((insn->insn_code & 0xf) != DR_V) {
	    set_defined(c, insn->opnds.bri.src);
	}
	break;
    case iclass_push:
	if (insn->opnds.a1.src != 0xffff) 
	    set_used(c, insn->opnds.a1.src);
	break;
    case iclass_pushi:
	break;
    case iclass_pushf:
	break;
    case iclass_nop:
	break;
    }
}

static void
reset_use_def_count(drisc_ctx c, virtual_insn *insns, virtual_mach_info vmi)
{

    int i;
    for(i=0; i< c->p->vreg_count; i++) {
	c->p->vregs[i].use_info.use_count = 0;
	c->p->vregs[i].use_info.def_count = 0;
    }
    apply_to_each(c, insns, vmi, do_use_def_count);
}
    
void
foreach_bit(bit_vec v, bv_func func, void *p1, void *p2)
{
    int i;
    for (i=0; i< v->len; i++) {
	int j;
	for (j = 0; j < 2; j++) {
	    unsigned char nibble = (v->vec[i] >> (j * 4)) & 0xf;
	    switch(nibble) {
	    case 0xf:
		func(i * 8 + j * 4 + 3, p1, p2);
		func(i * 8 + j * 4 + 2, p1, p2);
		func(i * 8 + j * 4 + 1, p1, p2);
		func(i * 8 + j * 4 + 0, p1, p2);
		break;
	    case 0xe:
		func(i * 8 + j * 4 + 3, p1, p2);
		func(i * 8 + j * 4 + 2, p1, p2);
		func(i * 8 + j * 4 + 1, p1, p2);
		break;
	    case 0xd:
		func(i * 8 + j * 4 + 3, p1, p2);
		func(i * 8 + j * 4 + 2, p1, p2);
		func(i * 8 + j * 4 + 0, p1, p2);
		break;
	    case 0xc:
		func(i * 8 + j * 4 + 3, p1, p2);
		func(i * 8 + j * 4 + 2, p1, p2);
		break;
	    case 0xb:
		func(i * 8 + j * 4 + 3, p1, p2);
		func(i * 8 + j * 4 + 1, p1, p2);
		func(i * 8 + j * 4 + 0, p1, p2);
		break;
	    case 0xa:
		func(i * 8 + j * 4 + 3, p1, p2);
		func(i * 8 + j * 4 + 1, p1, p2);
		break;
	    case 0x9:
		func(i * 8 + j * 4 + 3, p1, p2);
		func(i * 8 + j * 4 + 0, p1, p2);
		break;
	    case 0x8:
		func(i * 8 + j * 4 + 3, p1, p2);
		break;
	    case 0x7:
		func(i * 8 + j * 4 + 2, p1, p2);
		func(i * 8 + j * 4 + 1, p1, p2);
		func(i * 8 + j * 4 + 0, p1, p2);
		break;
	    case 0x6:
		func(i * 8 + j * 4 + 2, p1, p2);
		func(i * 8 + j * 4 + 1, p1, p2);
		break;
	    case 0x5:
		func(i * 8 + j * 4 + 2, p1, p2);
		func(i * 8 + j * 4 + 0, p1, p2);
		break;
	    case 0x4:
		func(i * 8 + j * 4 + 2, p1, p2);
		break;
	    case 0x3:
		func(i * 8 + j * 4 + 1, p1, p2);
		func(i * 8 + j * 4 + 0, p1, p2);
		break;
	    case 0x2:
		func(i * 8 + j * 4 + 1, p1, p2);
		break;
	    case 0x1:
		func(i * 8 + j * 4 + 0, p1, p2);
		break;
	    case 0x0:
		break;
	    }
	}
    }
}

static void
do_register_assign(drisc_ctx c, void *insns, void *code_end, int vlp,
		   virtual_mach_info vmi)
{
    int i;
    int itmpa, itmpb, dtmpa, dtmpb;

    if ((dr_getreg(c, &itmpa, DR_L, DR_TEMP) == 0) ||
	(dr_getreg(c, &itmpb, DR_L, DR_TEMP) == 0)) {
	fprintf(stderr, "Failure to get temporary regs in register assign\n");
    }
    dr_getreg(c, &dtmpa, DR_D, DR_TEMP);
    dr_getreg(c, &dtmpb, DR_D, DR_TEMP);
    for (i=0; i< DR_B; i++) {
	switch(i) {
	case DR_D:
	case DR_F:
	    c->p->v_tmps[i][0] = dtmpa;
	    c->p->v_tmps[i][1] = dtmpb;
	    c->p->v_tmps[i][2] = dtmpa;
	    break;
	default:
	    c->p->v_tmps[i][0] = itmpa;
	    c->p->v_tmps[i][1] = itmpb;
	    c->p->v_tmps[i][2] = c->p->machine_strr_tmp_reg;
	    break;
	}
    }
    
    for (i= 0; i < vmi->bbcount; i++) {
	basic_block bb = &vmi->bblist[i];
	int j;
	bb->reg_assigns = malloc(sizeof(short) * c->p->vreg_count);
	memset(bb->reg_assigns, 0xff, sizeof(short) * c->p->vreg_count);
	foreach_bit(bb->regs_defined, (bv_func) do_reg_assign, bb, c);
	for (j = 0 ; j < c->p->vreg_count; j++) {
	    if (bb->reg_assigns[j] != -1) {
		dr_putreg(c, bb->reg_assigns[j], c->p->vregs[j].typ);
	    }
	}
    }

    for(i=0; i< c->p->vreg_count; i++) {
	if (i == (vlp - 100) ) {
	    c->p->vregs[i].preg = c->dr_local_pointer;
	    c->p->vregs[i].offset = 0;
	} else if (c->p->vregs[i].typ == DR_B) {
	    /* block, size stored in offset value */
	    int size = c->p->vregs[i].offset;
	    c->p->vregs[i].preg = -1;
	    c->p->vregs[i].offset = dr_localb(c, size);
	} else {
	    c->p->vregs[i].preg = -1;
	    if (c->p->vregs[i].typ != DR_V) {
		c->p->vregs[i].offset = dr_local(c, c->p->vregs[i].typ);
	    } else {
		printf("internal void register error\n");
	    }
	}
    }
}

static int
preg_of(drisc_ctx c, basic_block bb, int vreg)
{
    if (vreg >= 100) {
	if (bb->reg_assigns[vreg - 100] == -1) {
	    return c->p->vregs[vreg - 100].preg;
	} else {
	    return bb->reg_assigns[vreg - 100];
	}
    } else {
	/* parameter */;
	if (c->p->c_param_args[vreg].is_register) {
	    return c->p->c_param_args[vreg].in_reg;
	}
	return -1;
    }
}

static int
offset_of(drisc_ctx c, int vreg)
{
    if (vreg >= 100) {
	return c->p->vregs[vreg - 100].offset;
    } else {
	/* parameter */;
	return c->p->c_param_args[vreg].offset;
    }
}

extern void
dr_virtual_lea(drisc_ctx c, int dest, int src)
{
    if (src < 100) {
	printf("error, attempt to do dr_virtual_lea on non-virtual\n");
	return;
    }
    if (type_of(c, src) != DR_B) {
	printf("error, attempt to do dr_virtual_lea on variable of type other than V_B\n");
	return;
    }
    dr_lea(c, dest, src, 0);
}

extern int
type_of(drisc_ctx c, int vreg)
{
    if (vreg >= 100) {
	return c->p->vregs[vreg - 100].typ;
    } else {
	return c->p->c_param_args[vreg].type;
    }
}

static int
tmp_for_vreg(drisc_ctx c, int tmp_num, int vreg)
{
    int typ = type_of(c, vreg);
    return c->p->v_tmps[typ][tmp_num];
}

static int
load_oprnd(drisc_ctx c, int tmp_num, int vreg)
{
    int offset = offset_of(c, vreg);
    int typ = type_of(c, vreg);
    int tmp = c->p->v_tmps[typ][tmp_num];
    if (vreg >= 100) {
	c->j->loadi(c, typ, 0, tmp, c->dr_local_pointer, offset);
    } else {
	c->j->loadi(c, typ, 0, tmp, c->dr_param_pointer, offset);
    }
    return tmp;
}    

static int
is_commutative(int insn_code)
{
    switch (insn_code) {
    case dr_jmp_addi:
    case dr_jmp_addu:
    case dr_jmp_addul:
    case dr_jmp_addl:
    case dr_jmp_addp:
    case dr_jmp_addf:
    case dr_jmp_addd:
    return 1;
    case dr_jmp_subi:
    case dr_jmp_subu:
    case dr_jmp_subul:
    case dr_jmp_subl:
    case dr_jmp_subp:
    case dr_jmp_subf:
    case dr_jmp_subd:
    return 0;
    case dr_jmp_muli:
    case dr_jmp_mulu:
    case dr_jmp_mulul:
    case dr_jmp_mull:
    case dr_jmp_mulf:
    case dr_jmp_muld:
    return 1;
    case dr_jmp_divi:
    case dr_jmp_divu:
    case dr_jmp_divul:
    case dr_jmp_divl:
    case dr_jmp_divf:
    case dr_jmp_divd:
    case dr_jmp_modi:
    case dr_jmp_modu:
    case dr_jmp_modul:
    case dr_jmp_modl:
    return 0;
    case dr_jmp_xori:
    case dr_jmp_xoru:
    case dr_jmp_xorul:
    case dr_jmp_xorl:
    case dr_jmp_andi:
    case dr_jmp_andu:
    case dr_jmp_andul:
    case dr_jmp_andl:
    case dr_jmp_ori:
    case dr_jmp_oru:
    case dr_jmp_orul:
    case dr_jmp_orl:
    return 1;
    case dr_jmp_lshi:
    case dr_jmp_lshu:
    case dr_jmp_lshul:
    case dr_jmp_lshl:
    case dr_jmp_rshi:
    case dr_jmp_rshu:
    case dr_jmp_rshul:
    case dr_jmp_rshl:
    return 0;
    default:
    printf("Unknown opcode in is_commutative\n");
    return 0;
    }
}

static int
store_oprnd(drisc_ctx c, int tmp_num, int vreg)
{
    int offset = offset_of(c, vreg);
    int typ = type_of(c, vreg);
    int tmp = c->p->v_tmps[typ][tmp_num];
    if (vreg >= 100) {
	c->j->storei(c, typ, 0, tmp, c->dr_local_pointer, offset);
    } else {
	c->j->storei(c, typ, 0, tmp, c->dr_param_pointer, offset);
    }
    return tmp;
}    

typedef struct label_translation {
    int old_location;
    int old_label;
    int new_label;
} *label_translation_table;

static int
get_new_label(int old_label, label_translation_table ltable)
{
    while (ltable->old_location != 0) {
	if (ltable->old_label == old_label) return ltable->new_label;
	ltable++;
    }
    printf("New label not found\n");
    return -1;
}

static void
emit_getreg(int bit, basic_block bb, drisc_ctx c)
{
    if (bb->reg_assigns[bit] != -1) {
	dr_rawget(c, bb->reg_assigns[bit], c->p->vregs[bit].typ, DR_VAR);
    }
}

static void
emit_putreg(int bit, basic_block bb, drisc_ctx c)
{
    if (bb->reg_assigns[bit] != -1) {
	dr_putreg(c, bb->reg_assigns[bit], c->p->vregs[bit].typ);
    }
}

static int count_verbose = -1;

static void
emit_insns(drisc_ctx c, void *insns, label_translation_table ltable,
	   virtual_mach_info vmi)
{
    int label_xlate = 0;
    int i, j = 0;
    virtual_insn *ip;
    if (count_verbose == -1) {
	count_verbose = (getenv ("DRISC_COUNTS") != NULL);
    }

    for (i = 0; i < vmi->bbcount; i++) {
	basic_block bb = &vmi->bblist[i];
	int last_dest_vreg = -1;
	int last_store_loc = 0;
	int insn_start = (long)c->p->cur_ip - (long)c->p->code_base;
	int insn_count = bb->end - bb->start;
	foreach_bit(bb->regs_defined, (bv_func) emit_getreg, bb, c);
	for (j = bb->start; j <= bb->end; j++) {
	    int loc;
	    ip = &((virtual_insn *)insns)[j];
	    loc = ((char*)ip - (char*)insns);
	    while ((loc != 0) && (ltable[label_xlate].old_location == loc)) {
		dr_label(c, ltable[label_xlate].new_label);
		label_xlate++;
	    }
	    if (c->drisc_debug) {
		printf("   v    ");
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    switch(ip->class_code) {
	    case iclass_arith3: {
		/* arith 3 operand integer insns */
		int dest_vreg = ip->opnds.a3.dest;
		int src1_vreg = ip->opnds.a3.src1;
		int src2_vreg = ip->opnds.a3.src2;
		int dest_preg;
		int src1_preg;
		int src2_preg;
		int insn_code = ip->insn_code;
		if ((last_dest_vreg == src2_vreg) && 
		    is_commutative(insn_code)) {
		    /* we only optimize opnd1, switch them */
		    src1_vreg = ip->opnds.a3.src2;
		    src2_vreg = ip->opnds.a3.src1;
		}
		dest_preg = preg_of(c, bb, dest_vreg);
		src1_preg = preg_of(c, bb, src1_vreg);
		src2_preg = preg_of(c, bb, src2_vreg);
		if (src1_preg == -1) {
		    /* load src1 */
		    if ((last_dest_vreg == src1_vreg) && 
			(tmp_for_vreg(c, 0, src1_vreg) != -1)) {
			if (has_single_def_use(c, src1_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src1_preg = tmp_for_vreg(c, 0, src1_vreg);
		    } else {
			src1_preg = load_oprnd(c, 0, src1_vreg);
		    }
		}
		if (src2_preg == -1) {
		    /* load src2 */
		    src2_preg = load_oprnd(c, 1, src2_vreg);
		}
		if (dest_preg == -1) {
		    dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		}
		( c->j->jmp_a3)[insn_code](c,
					   c->j->a3_data[insn_code].data1,
					   c->j->a3_data[insn_code].data2,
					   dest_preg, src1_preg, src2_preg);
		if (preg_of(c, bb, dest_vreg) == -1) {
		    /* no dest reg, store result */
		    last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
		    store_oprnd(c, 0, dest_vreg);
		    last_dest_vreg = dest_vreg;
		} else {
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_arith2: {
		/* arith 2 operand integer insns */
		int dest_vreg = ip->opnds.a2.dest;
		int src_vreg = ip->opnds.a2.src;
		int dest_preg = preg_of(c, bb, dest_vreg);
		int src_preg = preg_of(c, bb, src_vreg);
		int insn_code = ip->insn_code;
		if (src_preg == -1) {
		    /* load src */
		    if ((last_dest_vreg == src_vreg) && 
			(tmp_for_vreg(c, 0, src_vreg) != -1)) {
			if (has_single_def_use(c, src_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src_preg = tmp_for_vreg(c, 0, src_vreg);
		    } else {
			src_preg = load_oprnd(c, 0, src_vreg);
		    }
		}
		if (dest_preg == -1) {
		    dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		}
		( c->j->jmp_a2)[insn_code](c,
					   c->j->a2_data[insn_code].data1,
					   c->j->a2_data[insn_code].data2,
					   dest_preg, src_preg);
		if (preg_of(c, bb, dest_vreg) == -1) {
		    /* no dest reg, store result */
		    last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
		    store_oprnd(c, 0, dest_vreg);
		    last_dest_vreg = dest_vreg;
		} else {
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_arith3i:
	    {
		/* arith 3 immediate operand integer insns */
		int dest_vreg = ip->opnds.a3i.dest;
		int src_vreg = ip->opnds.a3i.src;
		long imm = ip->opnds.a3i.imm;
		int dest_preg = preg_of(c, bb, dest_vreg);
		int src_preg = preg_of(c, bb, src_vreg);
		int insn_code = ip->insn_code;
		if (src_preg == -1) {
		    /* load src */
		    if ((last_dest_vreg == src_vreg) && 
			(tmp_for_vreg(c, 0, src_vreg) != -1)) {
			if (has_single_def_use(c, src_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src_preg = tmp_for_vreg(c, 0, src_vreg);
		    } else {
			src_preg = load_oprnd(c, 0, src_vreg);
		    }
		}
		if (dest_preg == -1) {
		    dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		}
		( c->j->jmp_a3i)[insn_code](c,
					    c->j->a3i_data[insn_code].data1,
					    c->j->a3i_data[insn_code].data2,
					    dest_preg, src_preg, imm);
		if (preg_of(c, bb, dest_vreg) == -1) {
		    /* no dest reg, store result */
		    last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
		    store_oprnd(c, 0, dest_vreg);
		    last_dest_vreg = dest_vreg;
		} else {
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_ret:{
		/* arith  operand integer insns */
		int src_vreg = ip->opnds.a1.src;
		int src_preg = preg_of(c, bb, src_vreg);
		if (src_preg == -1) {
		    /* load src */
		    if ((last_dest_vreg == src_vreg) && 
			(tmp_for_vreg(c, 0, src_vreg) != -1)) {
			if (has_single_def_use(c, src_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src_preg = tmp_for_vreg(c, 0, src_vreg);
		    } else {
			src_preg = load_oprnd(c, 0, src_vreg);
		    }
		}
		( c->j->ret)(c, ip->insn_code, 0, src_preg);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_convert: {
		/* conversion insns */
		int dest_vreg = ip->opnds.a2.dest;
		int src_vreg = ip->opnds.a2.src;
		int dest_preg = preg_of(c, bb, dest_vreg);
		int src_preg = preg_of(c, bb, src_vreg);
		int from_type = (ip->insn_code >> 4) & 0xf;
		int to_type = ip->insn_code & 0xf;
		if (src_preg == -1) {
		    /* load src */
		    if ((last_dest_vreg == src_vreg)  && 
			(tmp_for_vreg(c, 0, src_vreg) != -1)) {
			if (has_single_def_use(c, src_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src_preg = tmp_for_vreg(c, 0, src_vreg);
		    } else {
			src_preg = load_oprnd(c, 0, src_vreg);
		    }
		}
		if (dest_preg == -1) {
		    dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		}
		( c->j->convert)(c, from_type, to_type, dest_preg, src_preg);
		if (preg_of(c, bb, dest_vreg) == -1) {
		    /* no dest reg, store result */
		    last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
		    store_oprnd(c, 0, dest_vreg);
		    last_dest_vreg = dest_vreg;
		} else {
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_loadstore: {
		/* load/store 3 operand integer insns */
		int dest_vreg = ip->opnds.a3.dest;
		int src1_vreg = ip->opnds.a3.src1;
		int src2_vreg = ip->opnds.a3.src2;
		int dest_preg;
		int src1_preg;
		int src2_preg;
		int store = ip->insn_code >> 4;
		int typ = ip->insn_code & 0xf;

		if (last_dest_vreg == src2_vreg) {
		    /* we only optimize opnd1, switch them */
		    /* load store is commutative */
		    src1_vreg = ip->opnds.a3.src2;
		    src2_vreg = ip->opnds.a3.src1;
		}
		dest_preg = preg_of(c, bb, dest_vreg);
		src1_preg = preg_of(c, bb, src1_vreg);
		src2_preg = preg_of(c, bb, src2_vreg);
		if (src1_preg == -1) {
		    /* load src1 */
		    if ((last_dest_vreg == src1_vreg)  && 
			(tmp_for_vreg(c, 0, src1_vreg) != -1)) {
			if (has_single_def_use(c, src1_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src1_preg = tmp_for_vreg(c, 0, src1_vreg);
		    } else {
			src1_preg = load_oprnd(c, 0, src1_vreg);
		    }
		}
		if (src2_preg == -1) {
		    /* load src2 */
		    src2_preg = load_oprnd(c, 1, src2_vreg);
		}
		if (store == 0) {
		    if (dest_preg == -1) {
			dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		    }
		    ( c->j->load)(c, typ, 0, dest_preg, src1_preg, src2_preg);
		    if (preg_of(c, bb, dest_vreg) == -1) {
			/* no dest reg, store result */
			last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
			store_oprnd(c, 0, dest_vreg);
			last_dest_vreg = dest_vreg;
		    } else {
			last_dest_vreg = -1;
		    }
		} else {
		    if (dest_preg == -1) {
			dest_preg = load_oprnd(c, 2, dest_vreg);
		    }
		    ( c->j->store)(c, typ, 0, dest_preg, src1_preg, src2_preg);
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_lea:
	    {
		/* load effective address */
		int dest_vreg = ip->opnds.a3i.dest;
		int src_vreg = ip->opnds.a3i.src;
		long imm = ip->opnds.a3i.imm;
		int dest_preg = preg_of(c, bb, dest_vreg);
		int src_preg = preg_of(c, bb, src_vreg);
		if (src_preg == -1) {
		    /* load src */
		    if ((last_dest_vreg == src_vreg) && 
			(tmp_for_vreg(c, 0, src_vreg) != -1)) {
			if (has_single_def_use(c, src_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src_preg = tmp_for_vreg(c, 0, src_vreg);
		    } else {
			src_preg = load_oprnd(c, 0, src_vreg);
		    }
		}
		if (dest_preg == -1) {
		    dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		}
		( c->j->lea)(c, 0, 0, dest_preg, src_preg, imm);
		if (preg_of(c, bb, dest_vreg) == -1) {
		    /* no dest reg, store result */
		    last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
		    store_oprnd(c, 0, dest_vreg);
		    last_dest_vreg = dest_vreg;
		} else {
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_loadstorei:
	    {
		/* load store immediate operand integer insns */
		int dest_vreg = ip->opnds.a3i.dest;
		int src_vreg = ip->opnds.a3i.src;
		long imm = ip->opnds.a3i.imm;
		int dest_preg = preg_of(c, bb, dest_vreg);
		int src_preg = preg_of(c, bb, src_vreg);
		int store = ip->insn_code >> 4;
		int typ = ip->insn_code & 0xf;
		if (src_preg == -1) {
		    /* load src */
		    if ((last_dest_vreg == src_vreg)  && 
			(tmp_for_vreg(c, 0, src_vreg) != -1)) {
			if (has_single_def_use(c, src_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src_preg = tmp_for_vreg(c, 0, src_vreg);
		    } else {
			src_preg = load_oprnd(c, 0, src_vreg);
		    }
		}
		if (store == 0) {
		    if (dest_preg == -1) {
			dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		    }
		    ( c->j->loadi)(c, typ, 0, dest_preg, src_preg, imm);
		    if (preg_of(c, bb, dest_vreg) == -1) {
			/* no dest reg, store result */
			last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
			store_oprnd(c, 0, dest_vreg);
			last_dest_vreg = dest_vreg;
		    } else {
			last_dest_vreg = -1;
		    }
		} else {
		    if (dest_preg == -1) {
			dest_preg = load_oprnd(c, 1, dest_vreg);
		    }
		    /* a store, dest is the source of the store */
		    ( c->j->storei)(c, typ, 0, dest_preg, src_preg, imm);
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_set:
	    {
		/* load store immediate operand integer insns */
		int dest_vreg = ip->opnds.a3i.dest;
		long imm = ip->opnds.a3i.imm;
		int dest_preg = preg_of(c, bb, dest_vreg);
		int typ = ip->insn_code & 0xf;
		if (dest_preg == -1) {
		    dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		}
		( c->j->set)(c, typ, 0, dest_preg, imm);
		if (preg_of(c, bb, dest_vreg) == -1) {
		    /* no dest reg, store result */
		    last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
		    store_oprnd(c, 0, dest_vreg);
		    last_dest_vreg = dest_vreg;
		} else {
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_setf:
	    {
		/* load store immediate operand integer insns */
		int dest_vreg = ip->opnds.sf.dest;
		double imm = ip->opnds.sf.imm;
		int dest_preg = preg_of(c, bb, dest_vreg);
		int typ = ip->insn_code & 0xf;
		if (dest_preg == -1) {
		    dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		}
		( c->j->setf)(c, typ, 0, dest_preg, imm);
		if (preg_of(c, bb, dest_vreg) == -1) {
		    /* no dest reg, store result */
		    last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
		    store_oprnd(c, 0, dest_vreg);
		    last_dest_vreg = dest_vreg;
		} else {
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_mov: {
		/* mov insns */
		int dest_vreg = ip->opnds.a2.dest;
		int src_vreg = ip->opnds.a2.src;
		int dest_preg = preg_of(c, bb, dest_vreg);
		int src_preg = preg_of(c, bb, src_vreg);
		int typ = ip->insn_code & 0xf;
		if (src_preg == -1) {
		    /* load src */
		    if ((last_dest_vreg == src_vreg) && 
			(tmp_for_vreg(c, 0, src_vreg) != -1)) {
			if (has_single_def_use(c, src_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src_preg = tmp_for_vreg(c, 0, src_vreg);
		    } else {
			src_preg = load_oprnd(c, 0, src_vreg);
		    }
		}
		if (dest_preg == -1) {
		    dest_preg = c->p->v_tmps[type_of(c, dest_vreg)][0];
		}
		( c->j->mov)(c, typ, 0, dest_preg, src_preg);
		if (preg_of(c, bb, dest_vreg) == -1) {
		    /* no dest reg, store result */
		    last_store_loc = (long)c->p->cur_ip - (long)c->p->code_base;
		    store_oprnd(c, 0, dest_vreg);
		    last_dest_vreg = dest_vreg;
		} else {
		    last_dest_vreg = -1;
		}
	    }
	    break;
	    case iclass_reti:
	    {
		/* return immediate integer insns */
		long imm = ip->opnds.a3i.imm;
		int typ = ip->insn_code & 0xf;
		( c->j->reti)(c, typ, 0, imm);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_branch:
	    {
		/* branch */
		int br_op = ip->insn_code;
		int label = get_new_label(ip->opnds.br.label, ltable);
		int src1_vreg = ip->opnds.br.src1;
		int src2_vreg = ip->opnds.br.src2;
		int src1_preg;
		int src2_preg;
/*		if (last_dest_vreg == src2_vreg) {
    # we should do this is we reverse the sense of the branch *
		    src1_vreg = ip->opnds.br.src2;
		    src2_vreg = ip->opnds.br.src1;
		}
*/
		src1_preg = preg_of(c, bb, src1_vreg);
		src2_preg = preg_of(c, bb, src2_vreg);
		if (src1_preg == -1) {
		    /* load src1 */
		    if ((last_dest_vreg == src1_vreg) && 
			(tmp_for_vreg(c, 0, src1_vreg) != -1)) {
			if (has_single_def_use(c, src1_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src1_preg = tmp_for_vreg(c, 0, src1_vreg);
		    } else {
			src1_preg = load_oprnd(c, 0, src1_vreg);
		    }
		}
		if (src2_preg == -1) {
		    /* load src2 */
		    src2_preg = load_oprnd(c, 1, src2_vreg);
		}
		( c->j->jmp_b)[br_op](c, c->j->b_data[br_op].data1,
				      c->j->b_data[br_op].data2,
				      src1_preg, src2_preg, label);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_branchi:
	    {
		/* branch immediate */
		int br_op = ip->insn_code;
		int label = get_new_label(ip->opnds.bri.label, ltable);
		int src1_vreg = ip->opnds.bri.src;
		long imm = ip->opnds.bri.imm;
		int src1_preg = preg_of(c, bb, src1_vreg);
		if (src1_preg == -1) {
		    /* load src1 */
		    if ((last_dest_vreg == src1_vreg) && 
			(tmp_for_vreg(c, 0, src1_vreg) != -1)) {
			if (has_single_def_use(c, src1_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src1_preg = tmp_for_vreg(c, 0, src1_vreg);
		    } else {
			src1_preg = load_oprnd(c, 0, src1_vreg);
		    }
		}
		
		( c->j->jmp_bi)[br_op](c, c->j->b_data[br_op].data1,
				       c->j->b_data[br_op].data2,
				       src1_preg, imm, label);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_jump_to_label:
	    {
		/* branch immediate */
		int label = get_new_label(ip->opnds.br.label, ltable);
		dr_jv(c, label);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_jump_to_reg:
	    {
		int src1_vreg = ip->opnds.br.src1;
		int src1_preg = preg_of(c, bb, src1_vreg);
		if (src1_preg == -1) {
		    /* load src1 */
		    if ((last_dest_vreg == src1_vreg) && 
			(tmp_for_vreg(c, 0, src1_vreg) != -1)) {
			if (has_single_def_use(c, src1_vreg)) {
			    if (c->drisc_debug) {
				printf(" -- Eliminating previous store -- \n");
			    }
			    c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			}
			src1_preg = tmp_for_vreg(c, 0, src1_vreg);
		    } else {
			src1_preg = load_oprnd(c, 0, src1_vreg);
		    }
		}
		
		dr_jp(c, src1_preg);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_jump_to_imm:
	    {
		long imm = ip->opnds.bri.imm;
		dr_jpi(c, imm);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_special:
		dr_special(c, ip->opnds.spec.type, ip->opnds.spec.param);
		break;
	    case iclass_call:
	    {
		basic_block next_bb = &vmi->bblist[i+1];
		int dest_vreg = ip->opnds.bri.src;
		int dest_preg = 0;
		
		long imm = ip->opnds.bri.imm;
		int reg = ip->insn_code & 0x10;
		int typ = ip->insn_code & 0xf;
		int rr;
		
		if (typ != DR_V) dest_preg = preg_of(c, next_bb, dest_vreg);
		if (reg != 0) {
		    int src1_vreg = imm;
		    int src1_preg = preg_of(c, bb, src1_vreg);
		    if (src1_preg == -1) {
			/* load src1 */
			if ((last_dest_vreg == src1_vreg) && 
			    (tmp_for_vreg(c, 0, src1_vreg) != -1)) {
			    if (has_single_def_use(c, src1_vreg)) {
				if (c->drisc_debug) {
				    printf(" -- Eliminating previous store -- \n");
				}
				c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			    }
			    src1_preg = tmp_for_vreg(c, 0, src1_vreg);
			} else {
			    src1_preg = load_oprnd(c, 0, src1_vreg);
			}
		    }
		    rr = dr_pcallr(c, typ, src1_preg);
		} else {
		    rr = dr_pcall(c, typ, (void*)imm);
		}
		if (typ != DR_V) {
		    if (preg_of(c, next_bb, dest_vreg) == -1) {
			/* no dest reg, store result */
			int offset = offset_of(c, dest_vreg);
			int typ = type_of(c, dest_vreg);
			if (dest_vreg >= 100) {
			    c->j->storei(c, typ, 0, rr, c->dr_local_pointer, 
					 offset);
			} else {
			    c->j->storei(c, typ, 0, rr, c->dr_param_pointer, 
					 offset);
			}
		    } else {
			/* move return value to result */
			c->j->mov(c, typ, 0, dest_preg, rr);
		    }
		}
		pushpop_inuse_regs(c, 1, ip);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_push:
	    {
		int typ = ip->insn_code & 0xf;
		int src1_vreg = ip->opnds.a1.src;
		int src1_preg;
		if (ip->opnds.a1.src != 0xffff) {
		    /* neg 1 used to signal push init */
		    src1_preg = preg_of(c, bb, src1_vreg);
		    if (src1_preg == -1) {
			/* load src1 */
			if ((last_dest_vreg == src1_vreg) && 
			    (tmp_for_vreg(c, 0, src1_vreg) != -1)) {
			    if (has_single_def_use(c, src1_vreg)) {
				if (c->drisc_debug) {
				    printf(" -- Eliminating previous store -- \n");
				}
				c->p->cur_ip = (char*)c->p->code_base + last_store_loc;
			    }
			    src1_preg = tmp_for_vreg(c, 0, src1_vreg);
			} else {
			    src1_preg = load_oprnd(c, 0, src1_vreg);
			}
		    }
		} else {
		    pushpop_inuse_regs(c, 0, ip);
		    src1_preg = -1;
		}
		dr_push_arg(c, typ, src1_preg);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_pushi:
	    {
		long imm = ip->opnds.a3i.imm;
		dr_push_argii(c, imm);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_pushf:
	    {
		int typ = ip->insn_code & 0xf;
		double imm = ip->opnds.sf.imm;
		c->j->pushfi(c, typ, imm);
		last_dest_vreg = -1;
	    }
	    break;
	    case iclass_nop:
		break;
	    }
	}
	foreach_bit(bb->regs_defined, (bv_func)emit_putreg, bb, c);
	if (count_verbose) {
	    int insn_end =  (long)c->p->cur_ip - (long)c->p->code_base;
	    printf("Basic Block %d, %d virtual instructions, %d physical instructions\n",
		   i, insn_count, c->j->count_insn(c, insn_start, insn_end));
	}
    }
    if ((unsigned)ltable[label_xlate].old_location == j * sizeof(virtual_insn)) {
	dr_label(c, ltable[label_xlate].new_label);
	label_xlate++;
    }
    if (ltable[label_xlate].old_location != 0) {
	printf("Some labels (%d, old loc %d) not placed\n", label_xlate,
	       ltable[label_xlate].old_location);
    }
}

static int
live_at_end(basic_block bb, int vreg)
{
    if (vreg >= 100) {
	return bit_vec_is_set(bb->live_at_end, vreg - 100);
    }
    return 1;
}

static int
get_tentative_assign(drisc_ctx c, int vreg, bit_vec vec)
{
    int tmp;
    dr_getreg(c, &tmp, type_of(c, vreg), DR_VAR);
    if (tmp != -1) {
	bit_vec_set(vec, tmp);
    }
    return tmp;
}

static void
put_unless(int bit, drisc_ctx c, int preg_assigned)
{
    int typ = preg_assigned >> 24;
    if (bit != (preg_assigned & 0xffffff)) {
	dr_putreg(c, bit, typ);
    }
}

static void
put_tentative_assigns(drisc_ctx c, int preg_assigned, bit_vec vec, int typ)
{
    foreach_bit(vec, (bv_func) put_unless, c, 
		(void*)(long)((preg_assigned&0xffffff) | (long)typ<<24));
}

typedef struct reg_state {
    drisc_ctx c;
    basic_block bb;
    preg_info *fpregs;
    preg_info *ipregs;
    int reg_count;
    int ret_reg;
    int ret_vreg;
    vreg_info *param_info;
} reg_state;

static void
init_reg_state(reg_state *state, drisc_ctx c)
{
    int i;
    state->fpregs = malloc(sizeof(preg_info));
    state->ipregs = malloc(sizeof(preg_info));
    state->reg_count = 0;
    state->ret_reg = -1;
    state->ret_vreg = -1;
    state->param_info = malloc(sizeof(vreg_info) * c->p->c_param_count);
    for (i=0; i< c->p->c_param_count; i++) {
	if (c->p->c_param_args[i].is_register) {
	    state->param_info[i].update_in_reg = 0;
	    state->param_info[i].value_in_mem = 0;
	    state->param_info[i].in_reg = c->p->c_param_args[i].in_reg;
	} else {	    
	    state->param_info[i].update_in_reg = 0;
	    state->param_info[i].value_in_mem = 1;
	    state->param_info[i].in_reg = -1;
	}
    }
}

static void
reset_reg_state(reg_state *state)
{
    int i;
    for (i=0; i < state->reg_count; i++) {
	state->fpregs[i].holds = state->ipregs[i].holds = -1;
    }
}

static int
update_in_reg(reg_state *s, int vreg)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	return vregs[vreg - 100].update_in_reg;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    return s->param_info[vreg].update_in_reg;
	}
    }
    return 0;
}

static void
set_update_in_reg(reg_state *s, int vreg, int value)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	vregs[vreg - 100].update_in_reg = value;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    s->param_info[vreg].update_in_reg = value;
	}
    }
}

static void
set_value_in_mem(reg_state *s, int vreg, int value)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	vregs[vreg - 100].value_in_mem = value;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    s->param_info[vreg].value_in_mem = value;
	}
    }
}

static int
value_in_mem(reg_state *s, int vreg)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	return vregs[vreg - 100].value_in_mem;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    return s->param_info[vreg].value_in_mem;
	}
    }
    return 0;
}

static void
set_in_reg(reg_state *s, int vreg, int value)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	vregs[vreg - 100].in_reg = value;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    s->param_info[vreg].in_reg = value;
	}
    }
}

static int
get_in_reg(reg_state *s, int vreg)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	return vregs[vreg - 100].in_reg;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    return s->param_info[vreg].in_reg;
	} else {
	    return s->c->p->c_param_args[vreg].in_reg;
	}
    }
}

static void
set_assign_loc(reg_state *s, int vreg, int value)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	vregs[vreg - 100].assign_loc = value;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    s->param_info[vreg].assign_loc = value;
	}
    }
}

static int
get_assign_loc(reg_state *s, int vreg)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	return vregs[vreg - 100].assign_loc;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    return s->param_info[vreg].assign_loc;
	}
    }
    return 0;
}

static void
set_last_use(reg_state *s, int vreg, int value)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	vregs[vreg - 100].last_use = value;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    s->param_info[vreg].last_use = value;
	}
    }
}

static int
get_last_use(reg_state *s, int vreg)
{
    vreg_info *vregs = s->c->p->vregs;
    if (vreg >= 100) {
	return vregs[vreg - 100].last_use;
    } else {
	if (!s->c->p->c_param_args[vreg].is_register) {
	    return s->param_info[vreg].last_use;
	}
    }
    return 0;
}

static void
spill_current_pregs(reg_state *state)
{
    drisc_ctx c = state->c;
    basic_block bb = state->bb;
    preg_info *pregs =state->ipregs;
    vreg_info *vregs = state->c->p->vregs;
    int i;
    int a;
    for (a = 0; a < 2 ; a++) {
	if (a == 1) {
	    pregs =state->fpregs;
	}

	for (i=0; i < state->reg_count; i++) {
	    int vreg = pregs[i].holds;
	    if (vreg >= 100) {
		if (update_in_reg(state, vreg) && live_at_end(bb, vreg)) {
		    int offset = offset_of(c, vreg);
		    int typ = type_of(c, vreg);
		    if (offset == 0xdeadbeef) {
			/* not previously spilled */
			/* cannot be parameter */
			offset = vregs[vreg-100].offset = dr_local(c, typ);
		    }
		    /* spill vreg to memory */
		    if (vreg >= 100) {
			c->j->storei(c, typ, 0, i, c->dr_local_pointer, offset);
		    } else {
			c->j->storei(c, typ, 0, i, c->dr_param_pointer, offset);
		    }
		    set_update_in_reg(state, vreg, 0);
		    set_value_in_mem(state, vreg, 1);
		    set_in_reg(state, vreg, -1);
		}
	    }
	}
    }
}


static int 
select_reg(reg_state *state, int vreg, int loc, int src)
{
    static int reg_debug = -1;
    int ret_reg = -1;
    int old_vreg = -1;
    vreg_info *vregs = state->c->p->vregs;
    drisc_ctx c = state->c;
    basic_block bb = state->bb;
    preg_info *pregs;
    preg_info *fpregs =state->fpregs;
    preg_info *ipregs =state->ipregs;
    switch (type_of(c, vreg)) {
    case DR_F: case DR_D:
	pregs = state->fpregs;
	break;
    default:
	pregs = state->ipregs;
	break;
    }
    if (reg_debug == -1) {
	reg_debug = (getenv("REG_DEBUG") != NULL);
    }
    if (vreg < 100) {
	/* parameter */
	if (c->p->c_param_args[vreg].is_register) {
	    return c->p->c_param_args[vreg].in_reg;
	}
    }
    if (vreg == 100) {
	return dr_lp(c);
    }
    if (type_of(c, vreg) == DR_B) {
	/* not really using this */
	return -1;
    }
    if (get_in_reg(state, vreg) != -1) {
	ret_reg = get_in_reg(state, vreg);
	set_assign_loc(state, vreg, loc);
	if (!src) set_update_in_reg(state, vreg, 1);
	return ret_reg;
    } else {
	int preg;
	struct bitv tmp_assigns[6];
	int tentative_assign = -1;
	init_bit_vec(&tmp_assigns[0], sizeof(tmp_assigns));
	if (reg_debug) printf("Get assignment for vreg %d\n", vreg);
	while ((preg = get_tentative_assign(c, vreg, &tmp_assigns[0]))
	       != -1) {
	    int used_vreg;
	    if (reg_debug) printf("checking preg %d\n", preg);
	    if (preg >= state->reg_count) {
		int i;
		fpregs = realloc(fpregs, sizeof(reg_state)*(preg+1));
		ipregs = realloc(ipregs, sizeof(reg_state)*(preg+1));
		state->fpregs = fpregs;
		state->ipregs = ipregs;
		for (i = state->reg_count; i <= preg; i++) {
		    ipregs[i].holds = fpregs[i].holds = -1;
		}
		switch (type_of(c, vreg)) {
		case DR_F: case DR_D:
		    pregs = state->fpregs;
		    break;
		default:
		    pregs = state->ipregs;
		    break;
		}
		state->reg_count = preg+1;
	    }
	    used_vreg = pregs[preg].holds;
	    if (used_vreg == -1) {
		ret_reg = preg;
		if (reg_debug) 
		    printf("preg %d empty, assigning to vreg %d\n",
			   preg, vreg);
		break;
	    }
	    if ((get_assign_loc(state, used_vreg) == loc) && src) {
		if (reg_debug) 
		    printf("preg %d assigned to vreg %d at this loc\n",
			   preg, used_vreg);
		continue;
	    }
	    if ((get_last_use(state, used_vreg) < loc) ||
		(!src && (get_last_use(state, used_vreg) == loc))) {
		ret_reg = preg;
		if (reg_debug) 
		    printf("preg %d assigned to vreg %d, but that vreg is done, assigning\n",
			   preg, used_vreg);
		break;
	    }
	    if (tentative_assign == -1) {
		tentative_assign = preg;
		if (reg_debug) 
		    printf("preg %d is new tentative assign\n", preg);
	    } else if (vregs[pregs[tentative_assign].holds-100].use_metric >
		       vregs[used_vreg-100].use_metric) {
		if (reg_debug) 
		    printf("preg %d is better tentative assign\n", preg);
		tentative_assign = preg;
	    } else {
		if (reg_debug) 
		    printf("previous assign %d was better tentative assign\n", 
			   tentative_assign);
	    }		    
	}
	if (ret_reg == -1) ret_reg = tentative_assign;
	put_tentative_assigns(c, -1, &tmp_assigns[0], 
			      type_of(c, vreg));
    }
    if (ret_reg != -1) old_vreg = pregs[ret_reg].holds;
    if (old_vreg != -1) {
	/* if this is not the same as the old assignment */
	if (((get_last_use(state, old_vreg) > loc) || live_at_end(bb, old_vreg))&&
	    update_in_reg(state, old_vreg))  {
	    int offset = offset_of(c, old_vreg);
	    int typ = type_of(c, old_vreg);
	    set_value_in_mem(state, old_vreg, 1);
	    set_update_in_reg(state, old_vreg, 0);
	    if (offset == 0xdeadbeef) {
		/* not previously spilled */
		offset = vregs[old_vreg-100].offset = dr_local(c, typ);
	    }
	    /* spill vreg to memory */
	    if (reg_debug) 
		printf("Spilling vreg %d to memory, new assignment is %d\n",
		       old_vreg, vreg);
	    if (old_vreg >= 100) {
		c->j->storei(c, typ, 0, ret_reg, c->dr_local_pointer, offset);
	    } else {
		c->j->storei(c, typ, 0, ret_reg, c->dr_param_pointer, offset);
	    }
	}
	set_in_reg(state, old_vreg, -1);
    }
    if (src && value_in_mem(state, vreg)) {
	int offset = offset_of(c, vreg);
	int typ = type_of(c, vreg);
	if (offset == 0xdeadbeef) {
	    /* not previously assigned */
	    printf("Virtual register %d used as source but not previously assigned \n", vreg);
	    offset = 0;
	}
	if (reg_debug) 
	    printf("Loading vreg %d from offset %d into preg %d\n",
		   vreg, offset, ret_reg);
	if (vreg >= 100) {
	    c->j->loadi(c, typ, 0, ret_reg, c->dr_local_pointer, offset);
	} else {
	    c->j->loadi(c, typ, 0, ret_reg, c->dr_param_pointer, offset);
	}
    } else {
	set_update_in_reg(state, vreg, 1);
    }
    if (ret_reg != -1) pregs[ret_reg].holds = vreg;
    set_in_reg(state, vreg, ret_reg);
    set_assign_loc(state, vreg, loc);
    if (src && (ret_reg == -1) && !value_in_mem(state,vreg)) {
	/* special x86 no float regs case */
	int offset = offset_of(c, vreg);
	int typ = type_of(c, vreg);
	if (reg_debug) 
	    printf("Special x86 loading vreg %d from offset %d into preg %d\n",
		   vreg, offset, ret_reg);
	if (vreg >= 100) {
	    c->j->loadi(c, typ, 0, ret_reg, c->dr_local_pointer, offset);
	} else {
	    c->j->loadi(c, typ, 0, ret_reg, c->dr_param_pointer, offset);
	}
    }
    return ret_reg;
}

static void
update_vreg_info(reg_state *s, basic_block bb, virtual_insn *ip, int loc)
{
    int i;
    int used[4];
    insn_uses(ip, &used[0]);
    used[3] = insn_defines(ip);
    for (i = 0; i < 4; i++) {
	if (used[i] == -1) continue;
	if (i < 3) {
	    set_last_use(s, used[i], loc);
	}
	if (value_in_mem(s, used[i]) == -1) {
	    set_value_in_mem(s, used[i], loc); /* first use */
	}
	if (used[i] >= 100) {
	    s->c->p->vregs[used[i]-100].use_metric++;
	}
    }
}

static void
new_emit_insns(drisc_ctx c, void *insns, label_translation_table ltable,
	   virtual_mach_info vmi)
{
    int label_xlate = 0;
    int i, j = 0;
    reg_state state;

    if (count_verbose == -1) {
	count_verbose = (getenv ("DRISC_COUNTS") != NULL);
    }
    init_reg_state(&state, c);
    state.c = c;
    for (j=0; j < c->p->vreg_count; j++) {
	if (type_of(c, 100 + j) == DR_B) {
	    /* offset is really size, fix that */
	    c->p->vregs[j].offset = dr_localb(c, c->p->vregs[j].offset);
	}
    }
    for (i = 0; i < vmi->bbcount; i++) {
	basic_block bb = &vmi->bblist[i];
	int insn_count = bb->end - bb->start;
	int insn_start;
	for (j=0; j < c->p->vreg_count; j++) {
	    c->p->vregs[j].assign_loc = -1;
	    c->p->vregs[j].in_reg = -1;
	    c->p->vregs[j].last_use = -1;
	    c->p->vregs[j].use_metric = 0;
	    c->p->vregs[j].update_in_reg = 0;
	    c->p->vregs[j].value_in_mem = -1;
	}
	for (j=0; j < c->p->c_param_count; j++) {
	    state.param_info[j].assign_loc = -1;
	    state.param_info[j].in_reg = -1;
	    state.param_info[j].last_use = -1;
	    state.param_info[j].use_metric = 0;
	    state.param_info[j].update_in_reg = 0;
	    state.param_info[j].value_in_mem = -1;
	}
	for (j = bb->start; j <= bb->end; j++) {
	    virtual_insn *ip = &((virtual_insn *)insns)[j];
	    update_vreg_info(&state, bb, ip, j);
	}
	for (j=0; j < c->p->vreg_count; j++) {
	    if (get_last_use(&state, j + 100) == -1) continue;
	    c->p->vregs[j].use_metric *= insn_count;
	    c->p->vregs[j].use_metric /= (c->p->vregs[j].last_use - 
					  c->p->vregs[j].value_in_mem +1);
	    c->p->vregs[j].value_in_mem = 
		bit_vec_is_set(bb->regs_used, j);
	}
	for (j=0; j < c->p->c_param_count; j++) {
	    if (c->p->c_param_args[j].is_register) {
		state.param_info[j].update_in_reg = 0;
		state.param_info[j].value_in_mem = 0;
		state.param_info[j].in_reg = c->p->c_param_args[j].in_reg;
	    } else {	    
		state.param_info[j].update_in_reg = 0;
		state.param_info[j].value_in_mem = 1;
		state.param_info[j].in_reg = -1;
	    }
	}
	reset_reg_state(&state);
	if (c->drisc_debug) {
	    printf("============= Starting basic block %d ===========\n", i);
	}
	insn_start = (long)c->p->cur_ip - (long)c->p->code_base;
	for (j = bb->start; j <= bb->end; j++) {
	    virtual_insn *ip;
	    int vused[3];
	    int vdest;
	    int pused[3];
	    int pdest = -1;
	    int loc;
	    int i;
	    int insn_code;
	    state.bb = bb;
	    ip = &((virtual_insn *)insns)[j];
	    insn_code = ip->insn_code;
	    loc = ((char*)ip - (char*)insns);
	    while ((loc != 0) && (ltable[label_xlate].old_location == loc)) {
		dr_label(c, ltable[label_xlate].new_label);
		label_xlate++;
	    }
	    if (c->drisc_debug) {
		printf("   v    loc(%d)  ", loc);
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    if (j == bb->end) {
		/* in case we branch out of this puppy */
		spill_current_pregs(&state);
	    }

	    insn_uses(ip, &vused[0]);
	    vdest = insn_defines(ip);
	    if (state.ret_vreg != -1) {
		if (c->p->vregs[state.ret_vreg - 100].last_use == j) {
		    /* only used here, use the ret ret */
		    c->p->vregs[state.ret_vreg - 100].in_reg = state.ret_reg;
		} else {
		    int tmp = select_reg(&state, state.ret_vreg, j, 0);
		    ( c->j->mov)(c, type_of(c, state.ret_vreg), 0, tmp, 
				 state.ret_reg);
		}
		state.ret_vreg = -1;
	    }
	    for(i=0; (i < 3) && (vused[i] != -1); i++) {
		pused[i] = select_reg(&state, vused[i], j, 1/*src*/);
	    }
	    if (vdest != -1) {
		pdest = select_reg(&state, vdest, j, 0/*dest*/);
	    }
	    if (c->drisc_debug && ((vdest != -1) || (vused[0] != -1))) {
		printf("\tvregs\t\t");
		if (vdest != -1) {
		    printf(" %c%d = ", OPND(vdest));
		    dr_dump_reg(c, type_of(c, vdest), pdest);
		    printf("(%d) - ", pdest);
		}
		for(i=0; (i < 3) && (vused[i] != -1); i++) {
		    printf(" %c%d = ", OPND(vused[i]));
		    dr_dump_reg(c, type_of(c, vused[i]), pused[i]);
		    printf("(%d) - ", pused[i]);
		}
		printf("\n");
	    }
	    switch(ip->class_code) {
	    case iclass_arith3:
		/* arith 3 operand integer insns */
		( c->j->jmp_a3)[insn_code](c,
					   c->j->a3_data[insn_code].data1,
					   c->j->a3_data[insn_code].data2,
					   pdest, pused[0], pused[1]);
		break;
	    case iclass_arith2: 
		( c->j->jmp_a2)[insn_code](c,
					   c->j->a2_data[insn_code].data1,
					   c->j->a2_data[insn_code].data2,
					   pdest, pused[0]);
		break;
	    case iclass_arith3i:
		/* arith 3 immediate operand integer insns */
		( c->j->jmp_a3i)[insn_code](c,
					    c->j->a3i_data[insn_code].data1,
					    c->j->a3i_data[insn_code].data2,
					    pdest, pused[0], 
					    ip->opnds.a3i.imm);
		break;
	    case iclass_ret:
		( c->j->ret)(c, ip->insn_code, 0, pused[0]);
		break;
	    case iclass_convert: 
		/* conversion insns */
		( c->j->convert)(c, (ip->insn_code >> 4) & 0xf, 
				 ip->insn_code & 0xf, pdest, pused[0]);
		break;
	    case iclass_loadstore: {
		/* load/store 3 operand integer insns */
		int store = ip->insn_code >> 4;
		int typ = ip->insn_code & 0xf;
		if (store == 0) {
		    ( c->j->load)(c, typ, 0, pdest, pused[0], pused[1]);
		} else {
		    ( c->j->store)(c, typ, 0, pused[2], pused[0], pused[1]);
		}
	    }
	    break;
	    case iclass_lea: {
		int offset = ip->opnds.a3i.imm + offset_of(c, vused[0]);
		int add_code = dr_jmp_addp;
		if (offset == 0) {
		    c->j->mov(c, DR_P, 0, pdest, dr_lp(c));
		} else {
		    (c->j->jmp_a3i)[add_code](c, 
					      c->j->a3i_data[add_code].data1,
					      c->j->a3i_data[add_code].data2,
					      pdest, dr_lp(c),  offset);
		}
		break;
	    }
	    case iclass_loadstorei:
		/* load store immediate operand integer insns */
		if ((ip->insn_code >> 4) == 0) {
		    ( c->j->loadi)(c, ip->insn_code & 0xf, 0, 
				   pdest, pused[0],
				   ip->opnds.a3i.imm);
		} else {
		    /* a store, dest is the source of the store */
		    ( c->j->storei)(c, ip->insn_code & 0xf, 0, 
				    pused[1], pused[0],
				    ip->opnds.a3i.imm);
		}
		break;
	    case iclass_set:
		/* load store immediate operand integer insns */
		( c->j->set)(c, ip->insn_code & 0xf, 0, pdest, 
			     ip->opnds.a3i.imm);
		break;
	    case iclass_setf:
		( c->j->setf)(c, ip->insn_code & 0xf, 0, pdest, 
			      ip->opnds.sf.imm);
		break;
	    case iclass_mov:
		( c->j->mov)(c, ip->insn_code & 0xf, 0, pdest, pused[0]);
		break;
	    case iclass_reti:
		/* return immediate integer insns */
		( c->j->reti)(c, ip->insn_code & 0xf, 0, ip->opnds.a3i.imm);
		break;
	    case iclass_branch:
	    {
		/* branch */
		int br_op = ip->insn_code;
		int label = get_new_label(ip->opnds.br.label, ltable);
		( c->j->jmp_b)[br_op](c, c->j->b_data[br_op].data1,
				      c->j->b_data[br_op].data2,
				      pused[0], pused[1], label);
	    }
	    break;
	    case iclass_branchi:
	    {
		/* branch immediate */
		int br_op = ip->insn_code;
		int label = get_new_label(ip->opnds.bri.label, ltable);
		long imm = ip->opnds.bri.imm;
		( c->j->jmp_bi)[br_op](c, c->j->b_data[br_op].data1,
				       c->j->b_data[br_op].data2,
				       pused[0], imm, label);
	    }
	    break;
	    case iclass_jump_to_label:
	    {
		/* branch immediate */
		int label = get_new_label(ip->opnds.br.label, ltable);
		dr_jv(c, label);
	    }
	    break;
	    case iclass_jump_to_reg:
		dr_jp(c, pused[0]);
		break;
	    case iclass_jump_to_imm:
		dr_jpi(c, ip->opnds.bri.imm);
		break;
	    case iclass_special:
		dr_special(c, ip->opnds.spec.type, ip->opnds.spec.param);
		break;
	    case iclass_call:
	    {
		long imm = ip->opnds.bri.imm;
		int reg = ip->insn_code & 0x10;
		int typ = ip->insn_code & 0xf;
		int rr;
		
		if (reg != 0) {
		    rr = dr_pcallr(c, typ, pused[0]);
		} else {
		    rr = dr_pcall(c, typ, (void*)imm);
		}
		state.ret_reg = rr;
		state.ret_vreg = vdest;
		pushpop_inuse_regs(c, 1, ip);
	    }
	    break;
	    case iclass_push:
	    {
		int typ = ip->insn_code & 0xf;
		if (ip->opnds.a1.src == 0xffff) {
		    pushpop_inuse_regs(c, 0, ip);
		    pused[0] = -1;
		}
		dr_push_arg(c, typ, pused[0]);
	    }
	    break;
	    case iclass_pushi:
		dr_push_argii(c, ip->opnds.a3i.imm);
		break;
	    case iclass_pushf:
		c->j->pushfi(c, ip->insn_code & 0xf, ip->opnds.sf.imm);
		break;
	    case iclass_nop:
		break;
	    }
	    if ((pdest == -1) && (vdest != -1)) {
		/* special x86 no float regs case */
		int offset = offset_of(c, vdest);
		int typ = type_of(c, vdest);
		set_value_in_mem(&state, vdest, 1);
		set_update_in_reg(&state, vdest, 0);
		if (offset == 0xdeadbeef) {
		    /* not previously spilled */
		    offset = c->p->vregs[vdest-100].offset = dr_local(c, typ);
		}
		/* spill vreg to memory */
		if (vdest >= 100) {
		    c->j->storei(c, typ, 0, -1, c->dr_local_pointer, offset);
		} else {
		    c->j->storei(c, typ, 0, -1, c->dr_param_pointer, offset);
		}
		set_in_reg(&state, vdest, -1);
		state.ret_vreg = -1;
	    }
	}
	spill_current_pregs(&state);
	if (count_verbose) {
	    int insn_end =  (long)c->p->cur_ip - (long)c->p->code_base;
	    printf("Basic Block %d, %d virtual instructions, %d physical instructions\n",
		   i, insn_count, c->j->count_insn(c, insn_start, insn_end));
	}
    }
    if ((unsigned)ltable[label_xlate].old_location == j * sizeof(virtual_insn)) {
	dr_label(c, ltable[label_xlate].new_label);
	label_xlate++;
    }
    if (ltable[label_xlate].old_location != 0) {
	int loc = ((char*)&((virtual_insn *)insns)[j]) - ((char*) insns);
	while (ltable[label_xlate].old_location == loc) {
	    dr_label(c, ltable[label_xlate].new_label);
	    label_xlate++;
	}
	if (ltable[label_xlate].old_location != 0) {
	    printf("Some labels2 (%d, old loc %d) not placed\n", label_xlate,
		   ltable[label_xlate].old_location);
	}
    }
    free(state.fpregs);
    free(state.ipregs);
}

static void
apply_to_each(drisc_ctx c, void *insns, virtual_mach_info vmi,
	      apply_func func)
{
    int i, j = 0;

    for (i = 0; i < vmi->bbcount; i++) {
	basic_block bb = &vmi->bblist[i];
	for (j = bb->start; j <= bb->end; j++) {
	    (func)(c, bb, insns, j);
	}
    }
}

static int 
const_prop_ip(drisc_ctx c, basic_block bb, virtual_insn *ip, virtual_insn *set_ip)
{
    int set_vreg = set_ip->opnds.a3i.dest;
    long set_imm = set_ip->opnds.a3i.imm;
    int set_typ = set_ip->insn_code & 0xf;
    int found = 0;
    switch(ip->class_code) {
    case iclass_arith3: 
    {
	/* arith 3 operand integer insns */
	int dest_vreg = ip->opnds.a3.dest;
	int src1_vreg = ip->opnds.a3.src1;
	int src2_vreg = ip->opnds.a3.src2;
	int insn_code = ip->insn_code;
	if ((src1_vreg == set_vreg) &&
	    is_commutative(insn_code)) {
	    src2_vreg = ip->opnds.a3.src1;
	    src1_vreg = ip->opnds.a3.src2;
	}
	if (src2_vreg == set_vreg) {
	    if (c->drisc_debug) {
		printf("   Replacing  ");
		virtual_print_insn(c, NULL, ip);
		printf(" with ");
	    }
	    ip->class_code = iclass_arith3i;
	    ip->opnds.a3i.dest = dest_vreg;
	    ip->opnds.a3i.src = src1_vreg;
	    ip->opnds.a3i.imm = set_imm;
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    set_ip->class_code = iclass_nop;
	    set_unused(c, bb, set_vreg);
	    found++;
	}
    }
    break;
    case iclass_arith2: 
    {
	/* arith 2 operand integer insns */
	int dest_vreg = ip->opnds.a2.dest;
	int src_vreg = ip->opnds.a2.src;
	int insn_code = ip->insn_code;
	if (src_vreg == set_vreg) {
	    if (c->drisc_debug) {
		printf("   Replacing  ");
		virtual_print_insn(c, NULL, ip);
		printf(" with ");
	    }
	    ip->class_code = iclass_set;
	    ip->opnds.a3i.dest = dest_vreg;
	    switch(insn_code) {
	    case dr_jmp_negi: case dr_jmp_negu: 
	    case dr_jmp_negl: case dr_jmp_negul:
		set_imm = -set_imm;
		break;
	    case dr_jmp_comi: case dr_jmp_comu: 
	    case dr_jmp_coml: case dr_jmp_comul:
		set_imm = ~set_imm;
		break;
	    case dr_jmp_noti: case dr_jmp_notu: 
	    case dr_jmp_notl: case dr_jmp_notul:
		set_imm = !set_imm;
		break;
	    default:
		assert(0);
		break;
	    }
	    ip->opnds.a3i.imm = set_imm;
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    set_ip->class_code = iclass_nop;
	    set_unused(c, bb, set_vreg);
	    found++;
	}
	break;
    }
    case iclass_arith3i:
    {
	/* arith 3 immediate operand integer insns */
	int dest_vreg = ip->opnds.a3i.dest;
	int src_vreg = ip->opnds.a3i.src;
	long imm = ip->opnds.a3i.imm;
	int insn_code = ip->insn_code;
	if (src_vreg == set_vreg) {
	    if (c->drisc_debug) {
		printf("   Replacing  ");
		virtual_print_insn(c, NULL, ip);
		printf(" with ");
	    }
	    switch (insn_code) {
	    case dr_jmp_addi:    case dr_jmp_addu:
	    case dr_jmp_addul:    case dr_jmp_addl:
	    case dr_jmp_addp:
		set_imm = set_imm + imm;
		break;
	    case dr_jmp_subi:    case dr_jmp_subu:
	    case dr_jmp_subul:    case dr_jmp_subl:
	    case dr_jmp_subp:
		set_imm = set_imm - imm;
		break;
	    case dr_jmp_muli:    case dr_jmp_mulu:
	    case dr_jmp_mulul:    case dr_jmp_mull:
		set_imm = set_imm * imm;
		break;
	    case dr_jmp_divi:    case dr_jmp_divu:
	    case dr_jmp_divul:    case dr_jmp_divl:
		set_imm = set_imm / imm;
		break;
	    case dr_jmp_modi:    case dr_jmp_modu:
	    case dr_jmp_modul:    case dr_jmp_modl:
		set_imm = set_imm % imm;
		break;
	    case dr_jmp_xori:    case dr_jmp_xoru:
	    case dr_jmp_xorul:    case dr_jmp_xorl:
		set_imm = set_imm ^ imm;
		break;
	    case dr_jmp_andi:    case dr_jmp_andu:
	    case dr_jmp_andul:    case dr_jmp_andl:
		set_imm = set_imm & imm;
		break;
	    case dr_jmp_ori:    case dr_jmp_oru:
	    case dr_jmp_orul:    case dr_jmp_orl:
		set_imm = set_imm | imm;
		break;
	    case dr_jmp_lshi:    case dr_jmp_lshu:
	    case dr_jmp_lshul:    case dr_jmp_lshl:
		set_imm = set_imm << imm;
		break;
	    case dr_jmp_rshi:    case dr_jmp_rshu:
	    case dr_jmp_rshul:    case dr_jmp_rshl:
		set_imm = set_imm >> imm;
		break;
	    default:
		assert(0);
	    }
	    ip->class_code = iclass_set;
	    ip->opnds.a3i.dest = dest_vreg;
	    ip->opnds.a3i.imm = set_imm;
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    set_ip->class_code = iclass_nop;
	    set_unused(c, bb, set_vreg);
	    found++;
	}
    }
    break;
    case iclass_ret:
    {
	/* arith  operand integer insns */
	int src_vreg = ip->opnds.a1.src;
	if (src_vreg == set_vreg) {
	    if (c->drisc_debug) {
		printf("   Replacing  ");
		virtual_print_insn(c, NULL, ip);
		printf(" with ");
	    }
	    ip->class_code = iclass_reti;
	    ip->insn_code = set_typ;
	    ip->opnds.a3i.imm = set_imm;
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    set_ip->class_code = iclass_nop;
	    set_unused(c, bb, set_vreg);
	    found++;
	}
    }
    break;
    case iclass_convert: {
	/* conversion insns */
	int dest_vreg = ip->opnds.a2.dest;
	int src_vreg = ip->opnds.a2.src;
	int from_type = (ip->insn_code >> 4) & 0xf;
	int to_type = ip->insn_code & 0xf;
	if (src_vreg == set_vreg) {
	    if (c->drisc_debug) {
		printf("   Replacing  ");
		virtual_print_insn(c, NULL, ip);
		printf(" with ");
	    }
	    switch(to_type) {
	    case DR_C: case DR_UC: 
	    case DR_S: case DR_US:
	    case DR_I: case DR_U:
	    case DR_L: case DR_UL:
		ip->class_code = iclass_set;
		ip->insn_code = to_type;
		ip->opnds.a3i.imm = set_imm;
		ip->opnds.a3i.dest = dest_vreg;
		break;
	    case DR_F: case DR_D:
		ip->class_code = iclass_setf;
		ip->insn_code = to_type;
		ip->opnds.sf.dest = dest_vreg;
		switch(from_type) {
		case DR_UC: case DR_US:
		case DR_U: case DR_UL:
		    ip->opnds.sf.imm = (double)
			(unsigned long) set_imm;
		    break;
		default:
		    ip->opnds.sf.imm = (double) set_imm;
		    break;
		}
	    }
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    set_ip->class_code = iclass_nop;
	    set_unused(c, bb, set_vreg);
	    found++;
	}
    }
	break;
    case iclass_loadstore: {
	/* load/store 3 operand integer insns */
	int dest_vreg = ip->opnds.a3.dest;
	int src1_vreg = ip->opnds.a3.src1;
	int src2_vreg = ip->opnds.a3.src2;
	
	if (set_vreg == src1_vreg) {
	    /* we only optimize src2_VREG, switch them */
	    /* load store is commutative */
	    src1_vreg = ip->opnds.a3.src2;
	    src2_vreg = ip->opnds.a3.src1;
	}
	if (src2_vreg == set_vreg) {
	    if (c->drisc_debug) {
		printf("   Replacing  ");
		virtual_print_insn(c, NULL, ip);
		printf(" with ");
	    }
	    ip->class_code = iclass_loadstorei;
	    ip->opnds.a3i.dest = dest_vreg;
	    ip->opnds.a3i.src = src1_vreg;
	    ip->opnds.a3i.imm = set_imm;
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    set_ip->class_code = iclass_nop;
	    set_unused(c, bb, set_vreg);
	    found++;
	}
    }
	break;
    case iclass_loadstorei:
	break;
    case iclass_lea:
	break;
    case iclass_set:
	break;
    case iclass_setf:
	break;
    case iclass_mov: {
	/* mov insns */
	int dest_vreg = ip->opnds.a2.dest;
	int src_vreg = ip->opnds.a2.src;
	if (src_vreg == set_vreg) {
	    if (c->drisc_debug) {
		printf("   Replacing  ");
		virtual_print_insn(c, NULL, ip);
		printf(" with ");
	    }
	    ip->class_code = iclass_set;
	    ip->opnds.a3i.dest = dest_vreg;
	    ip->opnds.a3i.imm = set_imm;
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    set_ip->class_code = iclass_nop;
	    set_unused(c, bb, set_vreg);
	    found++;
	}
    }
	break;
    case iclass_reti:
	break;
    case iclass_branch:
    {
	/* branch */
	int label = ip->opnds.br.label;
	int src1_vreg = ip->opnds.br.src1;
	int src2_vreg = ip->opnds.br.src2;
/*			    int br_op = ip->insn_code;
			    if (last_dest_vreg == src2_vreg) {
			    # we should do this is we reverse the sense of the branch *
			    src1_vreg = ip->opnds.br.src2;
			    src2_vreg = ip->opnds.br.src1;
			    }
*/
	if (src2_vreg == set_vreg) {
	    if (c->drisc_debug) {
		printf("   Replacing  ");
		virtual_print_insn(c, NULL, ip);
		printf(" with ");
	    }
	    ip->class_code = iclass_branchi;
	    ip->opnds.bri.src = src1_vreg;
	    ip->opnds.bri.label = label;
	    ip->opnds.bri.imm = set_imm;
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    set_ip->class_code = iclass_nop;
	    set_unused(c, bb, set_vreg);
	    found++;
	}
    }
    break;
    case iclass_branchi:
	break;
    case iclass_jump_to_label:
	break;
    case iclass_jump_to_reg:
	break;
    case iclass_jump_to_imm:
	break;
    case iclass_special:
	break;
    case iclass_call:
	break;
    case iclass_push:
    {
	int src1_vreg = ip->opnds.a1.src;
	if (src1_vreg == set_vreg) {
	    if (c->drisc_debug) {
		printf("   Replacing  ");
		virtual_print_insn(c, NULL, ip);
		printf(" with ");
	    }
	    ip->class_code = iclass_pushi;
	    ip->opnds.a3i.imm = set_imm;
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    set_ip->class_code = iclass_nop;
	    set_unused(c, bb, set_vreg);
	    found++;
	}
    }
    break;
    case iclass_pushi:
	break;
    case iclass_pushf:
	break;
    case iclass_nop:
	break;
    }
    return found;
}

static int
is_convert_noop(int insn_code)
{
    int from_type = (insn_code >> 4) & 0xf;
    int to_type = insn_code & 0xf;

    /* GSE -bug  This test should be for *generated* target, not host */
    if (sizeof(long) != sizeof(int)) return 0;
    switch(from_type) {
    case DR_I: case DR_U:
    case DR_L: case DR_UL:
	switch(to_type) {
	case DR_I: case DR_U:
	case DR_L: case DR_UL:
	    return 1;
	}
    }
    return 0;
}

static void
do_const_prop(drisc_ctx c, basic_block bb, virtual_insn *insns, int loc)
{
    virtual_insn *set_ip = &((virtual_insn *)insns)[loc];
    if ((set_ip->class_code == iclass_set) && 
	has_single_def_use(c, set_ip->opnds.a3i.dest)){
	
	int found = 0;
	int k;
	
	if (c->drisc_debug) {
	    printf(" Forward propagating    ");
	    virtual_print_insn(c, NULL, set_ip);
	    printf("\n");
	}
	for (k = loc + 1; ((k <= bb->end) && (!found)); k++) {
	    virtual_insn *ip = &((virtual_insn *)insns)[k];
	    found = const_prop_ip(c, bb, ip, set_ip);
	}
    } else if (((set_ip->class_code == iclass_mov) ||
		((set_ip->class_code == iclass_convert) &&
		 is_convert_noop(set_ip->insn_code))) && 
	       has_single_def_use(c, set_ip->opnds.a2.src)){
		/* back propagate move */
	int mov_src = set_ip->opnds.a2.src;
	int mov_dest = set_ip->opnds.a2.dest;
	int found = 0;
	int k;
	
	if (c->drisc_debug) {
	    printf(" mov propagating    ");
	    virtual_print_insn(c, NULL, set_ip);
	    printf("\n");
	}
	for (k = loc - 1; ((k > bb->start) && (!found)); k--) {
	    virtual_insn *ip = &((virtual_insn *)insns)[k];
	    switch(ip->class_code) {
	    case iclass_arith3: {
		/* arith 3 operand integer insns */
		int dest_vreg = ip->opnds.a3.dest;
		if (dest_vreg == mov_src) {
		    if (c->drisc_debug) {
			printf("   Replacing  ");
			virtual_print_insn(c, NULL, ip);
			printf(" with ");
		    }
		    ip->opnds.a3i.dest = mov_dest;
		    if (c->drisc_debug) {
			virtual_print_insn(c, NULL, ip);
			printf("\n");
			    }
		    set_ip->class_code = iclass_nop;
		    set_unused(c, bb, mov_src);
		    found++;
		}
	    }
	    break;
	    case iclass_arith2: {
		/* arith 2 operand integer insns */
		int dest_vreg = ip->opnds.a2.dest;
		if (dest_vreg == mov_src) {
		    if (c->drisc_debug) {
			printf("   Replacing  ");
			virtual_print_insn(c, NULL, ip);
			printf(" with ");
		    }
		    ip->opnds.a2.dest = mov_dest;
		    if (c->drisc_debug) {
			virtual_print_insn(c, NULL, ip);
			printf("\n");
		    }
		    set_ip->class_code = iclass_nop;
		    set_unused(c, bb, mov_src);
		    found++;
		}
		break;
	    }
	    case iclass_arith3i:
	    {
		/* arith 3 immediate operand integer insns */
		int dest_vreg = ip->opnds.a3i.dest;
		if (dest_vreg == mov_src) {
		    if (c->drisc_debug) {
			printf("   Replacing  ");
			virtual_print_insn(c, NULL, ip);
			printf(" with ");
		    }
		    ip->opnds.a3i.dest = mov_dest;
		    if (c->drisc_debug) {
			virtual_print_insn(c, NULL, ip);
			printf("\n");
		    }
		    set_ip->class_code = iclass_nop;
		    set_unused(c, bb, mov_src);
		    found++;
		}
	    }
	    break;
	    case iclass_ret:
		break;
	    case iclass_convert: {
		/* conversion insns */
		int dest_vreg = ip->opnds.a2.dest;
		if (dest_vreg == mov_src) {
		    if (c->drisc_debug) {
			printf("   Replacing  ");
			virtual_print_insn(c, NULL, ip);
			printf(" with ");
		    }
		    ip->opnds.a2.dest = mov_dest;
		    if (c->drisc_debug) {
			virtual_print_insn(c, NULL, ip);
			printf("\n");
		    }
		    set_ip->class_code = iclass_nop;
		    set_unused(c, bb, mov_src);
		    found++;
		}
	    }
	    break;
	    case iclass_loadstore: {
		int store = ip->insn_code >> 4;
		/* load/store 3 operand integer insns */
		int dest_vreg = ip->opnds.a3.dest;
		
		if (!store && (dest_vreg == mov_src)) {
		    if (c->drisc_debug) {
			printf("   Replacing  ");
			virtual_print_insn(c, NULL, ip);
			printf(" with ");
		    }
		    ip->opnds.a3i.dest = mov_src;
		    if (c->drisc_debug) {
			virtual_print_insn(c, NULL, ip);
			printf("\n");
		    }
		    set_ip->class_code = iclass_nop;
		    set_unused(c, bb, mov_src);
		    found++;
		}
	    }
	    break;
	    case iclass_loadstorei:
		break;
	    case iclass_lea:
		break;
	    case iclass_set:
		break;
	    case iclass_setf:
		break;
	    case iclass_mov: {
		/* mov insns */
		int dest_vreg = ip->opnds.a2.dest;
		if (dest_vreg == mov_src) {
		    if (c->drisc_debug) {
			printf("   Replacing  ");
			virtual_print_insn(c, NULL, ip);
			printf(" with ");
		    }
		    ip->opnds.a2.dest = mov_dest;
		    if (c->drisc_debug) {
			virtual_print_insn(c, NULL, ip);
			printf("\n");
		    }
		    set_ip->class_code = iclass_nop;
		    set_unused(c, bb, mov_src);
		    found++;
		}
	    }
	    break;
	    case iclass_reti:
	    case iclass_branch:
	    case iclass_branchi:
	    case iclass_jump_to_label:
	    case iclass_jump_to_reg:
	    case iclass_jump_to_imm:
	    case iclass_special:
	    case iclass_call:
	    case iclass_push:
	    case iclass_pushi:
	    case iclass_pushf:
	    case iclass_nop:
		break;
	    }
	}
    }
}

static void
const_propagation(drisc_ctx c, void *insns, virtual_mach_info vmi)
{
    apply_to_each(c, insns, vmi, do_const_prop);
}

static void
do_com_sub_exp(drisc_ctx c, basic_block bb, virtual_insn *insns, int loc)
{
    virtual_insn *root_insn = &((virtual_insn *)insns)[loc];
    int def_vreg = insn_defines(root_insn);
    int do_ldi0_optim = 0;
    int k;
    int stop = 0;
    int used_vregs[3];
    if (def_vreg == -1) return;
    insn_uses(root_insn, &used_vregs[0]);
    if (used_vregs[0] == -1) return;
    if ((root_insn->class_code == iclass_arith3) && 
	(root_insn->insn_code == dr_jmp_addp)) do_ldi0_optim = 1;
	    
    if (def_vreg != -1) {
	if ((used_vregs[0] == def_vreg) || (used_vregs[1] == def_vreg) ||
	    (used_vregs[2] == def_vreg)) return;
    }

    /* 
     * see if we can find an instruction just like (
     * except for dest) this ahead 
     */
    if (c->drisc_debug) {
	printf("Try to replace %d ", loc);
	virtual_print_insn(c, NULL, root_insn);
	printf("\n");
    }
    for (k = loc + 1; ((k <= bb->end) && (!stop)); k++) {
	virtual_insn *ip = &((virtual_insn *)insns)[k];
	int replace_vreg = insn_defines(ip);
	int l, stop2;
	/* stop looking if old dest is wiped */ 
	if (insn_define_test(ip, def_vreg)) stop++;
	/* stop looking if srcs become different */
	if (insn_define_test(ip, used_vregs[0])) stop++;
	if ((used_vregs[1] != -1) &&
	    insn_define_test(ip, used_vregs[1])) stop++;
	if ((used_vregs[2] != -1) &&
	    insn_define_test(ip, used_vregs[2])) stop++;
	
	if (do_ldi0_optim &&
	    (ip->class_code == iclass_loadstorei) &&
	    (ip->opnds.a3i.src == def_vreg) &&
	    (ip->opnds.a3i.imm == 0)) {
	    int use_target = 1;
	    /* 
	     * we've found a load immediate with an imm of 0 and with 
	     * a source that is the result of a prior add
	     * combine them.
	     */
	    if (def_vreg==used_vregs[0]) use_target++;
	    if (def_vreg==used_vregs[1]) use_target++;
	    if ((use_target != 0) && 
		(c->p->vregs[def_vreg-100].use_info.use_count != use_target)) {
		continue;
	    }
	    if (c->drisc_debug) {
		printf("load opt, changing  - ");
		virtual_print_insn(c, NULL, ip);
		printf(" - to - ");
	    }
	    ip->class_code = iclass_loadstore;
	    ip->opnds.a3.dest = ip->opnds.a3i.dest;
	    ip->opnds.a3.src1 = used_vregs[0];
	    ip->opnds.a3.src2 = used_vregs[1];
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, ip);
		printf("\n");
	    }
	    if (def_vreg >= 100) {
		c->p->vregs[def_vreg-100].use_info.use_count--;
	    }
	    if (c->p->vregs[def_vreg-100].use_info.use_count == (use_target-1)){
		c->p->vregs[def_vreg-100].use_info.def_count--;
		if (c->drisc_debug) {
		    printf("load opt eliminating  - ");
		    virtual_print_insn(c, NULL, root_insn);
		    printf("\n");
		}
		root_insn->class_code = iclass_nop;
		continue;
	    }
	}
	    
	if (replace_vreg == -1) continue;
	if (insn_use_test(ip, def_vreg)) do_ldi0_optim = 0;

	/* 
	 * insn is the same except for dest, make it a noop 
	 * and fixup uses of replace_vreg going forward 
	 */
	if (!insn_same_except_dest(ip, root_insn)) continue;
	if (live_at_end(bb, replace_vreg)) continue;
	if (c->drisc_debug) {
	    printf("eliminating  - - %d - ", k);
	    virtual_print_insn(c, NULL, ip);
	    printf("\n");
	}
	
	if (ip->class_code == iclass_loadstore) {
	    if (k > loc + 1) {
		virtual_insn *previous_ip = &((virtual_insn *)insns)[k - 1];
		if ((previous_ip->class_code == iclass_special) &&
		    (previous_ip->opnds.spec.type == DR_SEGMENTED_FOLLOWS)) {
		    /* 
		     * We're eliminating a loadstore and previous was a 
		     * special SEGMENTED_FOLLOWS, kill the special too.
		     */
		    previous_ip->class_code = iclass_nop;
		}
	    }
	}
	    
	ip->class_code = iclass_nop;
	stop2 = 0;
	for (l = k + 1; ((l <= bb->end) && (!stop2)); l++) {
	    virtual_insn *sub_ip = &((virtual_insn *)insns)[l];
	    if (insn_define_test(sub_ip, replace_vreg)) {
		stop2++;
	    }
	    if (!insn_use_test(sub_ip, replace_vreg)) continue;
	    if (c->drisc_debug) {
		printf("   Reg %d forward subst - Replacing %d ", 
		       def_vreg, l);
		virtual_print_insn(c, NULL, sub_ip);
		printf(" with ");
	    }
	    replace_insn_src(sub_ip, replace_vreg, def_vreg);
	    set_used(c, def_vreg);
	    if (c->drisc_debug) {
		virtual_print_insn(c, NULL, sub_ip);
		printf("\n");
	    }
	}
    }
    if (c->drisc_debug) {
	printf("stopped \n");
    }
}

static void
CSE_elimination(drisc_ctx c, void *insns, virtual_mach_info vmi)
{
    apply_to_each(c, insns, vmi, do_com_sub_exp);
}

extern void
virtual_proc_start(drisc_ctx c, char *subr_name, int arg_count,
		   arg_info_list args, dr_reg_t *arglist)
{
    int i;
    virtual_mach_info vmi = (virtual_mach_info)c->p->mach_info;
    vmi->arg_info = args;
    for (i = 0; i < arg_count; i++) {
	c->p->c_param_args[i].in_reg = i;
    }
}

extern void dr_virtual_init(drisc_ctx c);

static label_translation_table
build_label_translation(drisc_ctx c)
{
    int i;
    int label_count = c->p->branch_table.next_label;
    label_translation_table l = malloc(sizeof(struct label_translation) *
				       (label_count + 1));
    for(i = 0; i < label_count; i++) {
	l[i].old_label = i;
	l[i].old_location = c->p->branch_table.label_locs[i];
	l[i].new_label = dr_genlabel(c);
    }
    /* Good old reliable insertion sort */
    {
      int i, curIdx;
      struct label_translation tmp;

      for(curIdx = 0; curIdx < label_count; curIdx++) {
        for(i = curIdx+1; i < label_count; i++) {
          if(l[curIdx].old_location > l[i].old_location) {
            /* swap them */
            tmp = l[curIdx];
            l[curIdx] = l[i];
            l[i] = tmp;
          }
        }
      }
    }
    l[label_count].old_location = 0;
    return l;
}

static int
virtual_insn_count(drisc_ctx c)
{
    virtual_insn *insn = c->p->code_base;
    int count = 0;
    for (; insn < (virtual_insn *)c->p->cur_ip; insn++) {
	if (insn->class_code != iclass_nop) count++;
    }
    return count;
}
	    
extern void
virtual_end(drisc_ctx c)
{
    static int no_optimize = -1;
    static int drisc_verbose = -1;
    static int old_reg_alloc = -1;
    static int do_emulation = -1;

    virtual_mach_info vmi = (virtual_mach_info)c->p->mach_info;
    void *insns = c->p->code_base;
    void *code_end = c->p->cur_ip;
    label_translation_table ltable;
    int virtual_local_pointer = c->dr_local_pointer;

    if (drisc_verbose == -1) {
	drisc_verbose = (getenv ("DRISC_VERBOSE") != NULL);
	no_optimize = (getenv ("DRISC_NOOPTIMIZE") != NULL);
	old_reg_alloc = (getenv ("DRISC_OLD_REGS") != NULL);
	do_emulation = (getenv("DRISC_DO_EMULATION") != NULL);
#ifdef EMULATION_ONLY
	do_emulation = 1;
#endif
    }

    build_bbs(c, insns, code_end);

    if (!no_optimize) {
	if (count_verbose == -1) {
	    count_verbose = (getenv ("DRISC_COUNTS") != NULL);
	}
	if (count_verbose == 1) {
	    printf("Prior to optimization, %d non-null virtual insns\n", 
		   virtual_insn_count(c));
	}
	const_propagation(c, insns, vmi);
	if (count_verbose == 1) {
	    printf("After constant propagation, %d non-null virtual insns\n", 
		   virtual_insn_count(c));
	}
	CSE_elimination(c, insns, vmi);
	if (count_verbose == 1) {
	    printf("After duplicate instruction elimination (CSE-lite), %d non-null virtual insns\n", 
		   virtual_insn_count(c));
	}
	reset_use_def_count(c, insns, vmi);
    }

    if (drisc_verbose) {
	dump_bbs(c);
	c->drisc_debug = 1;
    }
    c->p->virtual.mach_jump = c->j;
    c->p->virtual.mach_reset = c->p->mach_reset;
    c->p->virtual.mach_info = c->p->mach_info;
    c->p->virtual.code_base = c->p->code_base;
    c->p->virtual.cur_ip = c->p->cur_ip;
    c->p->virtual.code_limit = c->p->code_limit;

    if (do_emulation) {
	setup_VM_proc(c);
	c->p->mach_reset = dr_virtual_init;
    } else {
	c->j = c->p->native.mach_jump;
	c->p->mach_reset = c->p->native.mach_reset;
	c->p->mach_info = c->p->native.mach_info;
	c->p->code_base = c->p->native.code_base;
	c->p->native.code_base = NULL;
	c->p->native.mach_info = NULL;
	c->p->cur_ip = c->p->code_base;
	c->p->code_limit = c->p->native.code_limit;

	c->p->native_mach_reset(c);
	(c->j->proc_start)(c, "no name", c->p->c_param_count, 
			   vmi->arg_info, (void*)0);
	ltable = build_label_translation(c);
	if (old_reg_alloc) {
	    do_register_assign(c, insns, code_end, virtual_local_pointer, vmi);
	    emit_insns(c, insns, ltable, vmi);
	} else {
	    new_emit_insns(c, insns, ltable, vmi);
	}
	free_bbs(vmi);
	free(ltable);
	dr_end(c);
	c->j = c->p->native.mach_jump;
	c->p->native.mach_reset = c->p->mach_reset;
	c->p->native.mach_info = c->p->mach_info;
	c->p->native.code_base = c->p->code_base;
	c->p->native.cur_ip = c->p->cur_ip;
	c->p->native.code_limit = c->p->code_limit;
	c->p->code_base = NULL;
	c->p->mach_info = NULL;
	c->p->mach_reset = dr_virtual_init;
    }
}

