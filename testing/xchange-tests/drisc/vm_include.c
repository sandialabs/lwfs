/* This file is generated from virtual.ops.  Do not edit directly. */

void emulate_arith3(int code, struct reg_type *dest, struct reg_type *src1, struct reg_type *src2)
{
    switch(code) {
	  case dr_jmp_addi:  dest->u.i.i =  src1->u.i.i +  src2->u.i.i; break;
	  case dr_jmp_addu:  dest->u.u.u =  src1->u.u.u +  src2->u.u.u; break;
	  case dr_jmp_addul:  dest->u.ul.ul =  src1->u.ul.ul +  src2->u.ul.ul; break;
	  case dr_jmp_addl:  dest->u.l.l =  src1->u.l.l +  src2->u.l.l; break;
	  case dr_jmp_addp:  dest->u.p.p = (char*) src1->u.p.p + (long) src2->u.p.p; break;
	  case dr_jmp_subi:  dest->u.i.i =  src1->u.i.i -  src2->u.i.i; break;
	  case dr_jmp_subu:  dest->u.u.u =  src1->u.u.u -  src2->u.u.u; break;
	  case dr_jmp_subul:  dest->u.ul.ul =  src1->u.ul.ul -  src2->u.ul.ul; break;
	  case dr_jmp_subl:  dest->u.l.l =  src1->u.l.l -  src2->u.l.l; break;
	  case dr_jmp_subp:  dest->u.p.p = (char*) src1->u.p.p - (long) src2->u.p.p; break;
	  case dr_jmp_mulu:  dest->u.u.u =  src1->u.u.u *  src2->u.u.u; break;
	  case dr_jmp_mulul:  dest->u.ul.ul =  src1->u.ul.ul *  src2->u.ul.ul; break;
	  case dr_jmp_muli:  dest->u.i.i =  src1->u.i.i *  src2->u.i.i; break;
	  case dr_jmp_mull:  dest->u.l.l =  src1->u.l.l *  src2->u.l.l; break;
	  case dr_jmp_divu:  dest->u.u.u =  src1->u.u.u /  src2->u.u.u; break;
	  case dr_jmp_divul:  dest->u.ul.ul =  src1->u.ul.ul /  src2->u.ul.ul; break;
	  case dr_jmp_divi:  dest->u.i.i =  src1->u.i.i /  src2->u.i.i; break;
	  case dr_jmp_divl:  dest->u.l.l =  src1->u.l.l /  src2->u.l.l; break;
	  case dr_jmp_modu:  dest->u.u.u =  src1->u.u.u %  src2->u.u.u; break;
	  case dr_jmp_modul:  dest->u.ul.ul =  src1->u.ul.ul %  src2->u.ul.ul; break;
	  case dr_jmp_modi:  dest->u.i.i =  src1->u.i.i %  src2->u.i.i; break;
	  case dr_jmp_modl:  dest->u.l.l =  src1->u.l.l %  src2->u.l.l; break;
	  case dr_jmp_andu:  dest->u.u.u =  src1->u.u.u &  src2->u.u.u; break;
	  case dr_jmp_andul:  dest->u.ul.ul =  src1->u.ul.ul &  src2->u.ul.ul; break;
	  case dr_jmp_andi:  dest->u.i.i =  src1->u.i.i &  src2->u.i.i; break;
	  case dr_jmp_andl:  dest->u.l.l =  src1->u.l.l &  src2->u.l.l; break;
	  case dr_jmp_oru:  dest->u.u.u =  src1->u.u.u |  src2->u.u.u; break;
	  case dr_jmp_orul:  dest->u.ul.ul =  src1->u.ul.ul |  src2->u.ul.ul; break;
	  case dr_jmp_ori:  dest->u.i.i =  src1->u.i.i |  src2->u.i.i; break;
	  case dr_jmp_orl:  dest->u.l.l =  src1->u.l.l |  src2->u.l.l; break;
	  case dr_jmp_xoru:  dest->u.u.u =  src1->u.u.u ^  src2->u.u.u; break;
	  case dr_jmp_xorul:  dest->u.ul.ul =  src1->u.ul.ul ^  src2->u.ul.ul; break;
	  case dr_jmp_xori:  dest->u.i.i =  src1->u.i.i ^  src2->u.i.i; break;
	  case dr_jmp_xorl:  dest->u.l.l =  src1->u.l.l ^  src2->u.l.l; break;
	  case dr_jmp_lshu:  dest->u.u.u =  src1->u.u.u <<  src2->u.u.u; break;
	  case dr_jmp_lshul:  dest->u.ul.ul =  src1->u.ul.ul <<  src2->u.ul.ul; break;
	  case dr_jmp_lshi:  dest->u.i.i =  src1->u.i.i <<  src2->u.i.i; break;
	  case dr_jmp_lshl:  dest->u.l.l =  src1->u.l.l <<  src2->u.l.l; break;
	  case dr_jmp_rshu:  dest->u.u.u =  src1->u.u.u >>  src2->u.u.u; break;
	  case dr_jmp_rshul:  dest->u.ul.ul =  src1->u.ul.ul >>  src2->u.ul.ul; break;
	  case dr_jmp_rshi:  dest->u.i.i =  src1->u.i.i >>  src2->u.i.i; break;
	  case dr_jmp_rshl:  dest->u.l.l =  src1->u.l.l >>  src2->u.l.l; break;
	  case dr_jmp_addf:  dest->u.f.f =  src1->u.f.f +  src2->u.f.f; break;
	  case dr_jmp_addd:  dest->u.d.d =  src1->u.d.d +  src2->u.d.d; break;
	  case dr_jmp_subf:  dest->u.f.f =  src1->u.f.f -  src2->u.f.f; break;
	  case dr_jmp_subd:  dest->u.d.d =  src1->u.d.d -  src2->u.d.d; break;
	  case dr_jmp_mulf:  dest->u.f.f =  src1->u.f.f *  src2->u.f.f; break;
	  case dr_jmp_muld:  dest->u.d.d =  src1->u.d.d *  src2->u.d.d; break;
	  case dr_jmp_divf:  dest->u.f.f =  src1->u.f.f /  src2->u.f.f; break;
	  case dr_jmp_divd:  dest->u.d.d =  src1->u.d.d /  src2->u.d.d; break;
    }
}
void emulate_arith3i(int code, struct reg_type *dest, struct reg_type *src1, long imm)
{
    switch(code) {
	  case dr_jmp_addi:  dest->u.i.i =  src1->u.i.i +  imm; break;
	  case dr_jmp_addu:  dest->u.u.u =  src1->u.u.u +  imm; break;
	  case dr_jmp_addul:  dest->u.ul.ul =  src1->u.ul.ul +  imm; break;
	  case dr_jmp_addl:  dest->u.l.l =  src1->u.l.l +  imm; break;
	  case dr_jmp_addp:  dest->u.p.p = (char*) src1->u.p.p + (long) imm; break;
	  case dr_jmp_subi:  dest->u.i.i =  src1->u.i.i -  imm; break;
	  case dr_jmp_subu:  dest->u.u.u =  src1->u.u.u -  imm; break;
	  case dr_jmp_subul:  dest->u.ul.ul =  src1->u.ul.ul -  imm; break;
	  case dr_jmp_subl:  dest->u.l.l =  src1->u.l.l -  imm; break;
	  case dr_jmp_subp:  dest->u.p.p = (char*) src1->u.p.p - (long) imm; break;
	  case dr_jmp_mulu:  dest->u.u.u =  src1->u.u.u *  imm; break;
	  case dr_jmp_mulul:  dest->u.ul.ul =  src1->u.ul.ul *  imm; break;
	  case dr_jmp_muli:  dest->u.i.i =  src1->u.i.i *  imm; break;
	  case dr_jmp_mull:  dest->u.l.l =  src1->u.l.l *  imm; break;
	  case dr_jmp_divu:  dest->u.u.u =  src1->u.u.u /  imm; break;
	  case dr_jmp_divul:  dest->u.ul.ul =  src1->u.ul.ul /  imm; break;
	  case dr_jmp_divi:  dest->u.i.i =  src1->u.i.i /  imm; break;
	  case dr_jmp_divl:  dest->u.l.l =  src1->u.l.l /  imm; break;
	  case dr_jmp_modu:  dest->u.u.u =  src1->u.u.u %  imm; break;
	  case dr_jmp_modul:  dest->u.ul.ul =  src1->u.ul.ul %  imm; break;
	  case dr_jmp_modi:  dest->u.i.i =  src1->u.i.i %  imm; break;
	  case dr_jmp_modl:  dest->u.l.l =  src1->u.l.l %  imm; break;
	  case dr_jmp_andi:  dest->u.i.i =  src1->u.i.i &  imm; break;
	  case dr_jmp_andu:  dest->u.u.u =  src1->u.u.u &  imm; break;
	  case dr_jmp_andul:  dest->u.ul.ul =  src1->u.ul.ul &  imm; break;
	  case dr_jmp_andl:  dest->u.l.l =  src1->u.l.l &  imm; break;
	  case dr_jmp_ori:  dest->u.i.i =  src1->u.i.i |  imm; break;
	  case dr_jmp_oru:  dest->u.u.u =  src1->u.u.u |  imm; break;
	  case dr_jmp_orul:  dest->u.ul.ul =  src1->u.ul.ul |  imm; break;
	  case dr_jmp_orl:  dest->u.l.l =  src1->u.l.l |  imm; break;
	  case dr_jmp_xori:  dest->u.i.i =  src1->u.i.i ^  imm; break;
	  case dr_jmp_xoru:  dest->u.u.u =  src1->u.u.u ^  imm; break;
	  case dr_jmp_xorul:  dest->u.ul.ul =  src1->u.ul.ul ^  imm; break;
	  case dr_jmp_xorl:  dest->u.l.l =  src1->u.l.l ^  imm; break;
	  case dr_jmp_lshi:  dest->u.i.i =  src1->u.i.i <<  imm; break;
	  case dr_jmp_lshu:  dest->u.u.u =  src1->u.u.u <<  imm; break;
	  case dr_jmp_lshul:  dest->u.ul.ul =  src1->u.ul.ul <<  imm; break;
	  case dr_jmp_lshl:  dest->u.l.l =  src1->u.l.l <<  imm; break;
	  case dr_jmp_rshi:  dest->u.i.i =  src1->u.i.i >>  imm; break;
	  case dr_jmp_rshu:  dest->u.u.u =  src1->u.u.u >>  imm; break;
	  case dr_jmp_rshul:  dest->u.ul.ul =  src1->u.ul.ul >>  imm; break;
	  case dr_jmp_rshl:  dest->u.l.l =  src1->u.l.l >>  imm; break;
    }
}
void emulate_arith2(int code, struct reg_type *dest, struct reg_type *src)
{
    switch(code) {
	  case dr_jmp_comi:  dest->u.i.i = ~  src->u.i.i; break;
	  case dr_jmp_comu:  dest->u.u.u = ~  src->u.u.u; break;
	  case dr_jmp_comul:  dest->u.ul.ul = ~  src->u.ul.ul; break;
	  case dr_jmp_coml:  dest->u.l.l = ~  src->u.l.l; break;
	  case dr_jmp_noti:  dest->u.i.i = !  src->u.i.i; break;
	  case dr_jmp_notu:  dest->u.u.u = !  src->u.u.u; break;
	  case dr_jmp_notul:  dest->u.ul.ul = !  src->u.ul.ul; break;
	  case dr_jmp_notl:  dest->u.l.l = !  src->u.l.l; break;
	  case dr_jmp_negi:  dest->u.i.i = -  src->u.i.i; break;
	  case dr_jmp_negu:  dest->u.u.u = -  src->u.u.u; break;
	  case dr_jmp_negul:  dest->u.ul.ul = -  src->u.ul.ul; break;
	  case dr_jmp_negl:  dest->u.l.l = -  src->u.l.l; break;
	  case dr_jmp_negf:  dest->u.f.f = -  src->u.f.f; break;
	  case dr_jmp_negd:  dest->u.d.d = -  src->u.d.d; break;
    }
}
int emulate_branch(int code, struct reg_type *src1, struct reg_type *src2)
{
    switch(code) {
	  case dr_jmp_beqi:  return ( src1->u.i.i) == ( src2->u.i.i);
	  case dr_jmp_bequ:  return ( src1->u.u.u) == ( src2->u.u.u);
	  case dr_jmp_bequl:  return ( src1->u.ul.ul) == ( src2->u.ul.ul);
	  case dr_jmp_beql:  return ( src1->u.l.l) == ( src2->u.l.l);
	  case dr_jmp_beqp:  return ((long) src1->u.p.p) == ((long) src2->u.p.p);
	  case dr_jmp_beqd:  return ( src1->u.d.d) == ( src2->u.d.d);
	  case dr_jmp_beqf:  return ( src1->u.f.f) == ( src2->u.f.f);
	  case dr_jmp_bgei:  return ( src1->u.i.i) >= ( src2->u.i.i);
	  case dr_jmp_bgeu:  return ( src1->u.u.u) >= ( src2->u.u.u);
	  case dr_jmp_bgeul:  return ( src1->u.ul.ul) >= ( src2->u.ul.ul);
	  case dr_jmp_bgel:  return ( src1->u.l.l) >= ( src2->u.l.l);
	  case dr_jmp_bgep:  return ((long) src1->u.p.p) >= ((long) src2->u.p.p);
	  case dr_jmp_bged:  return ( src1->u.d.d) >= ( src2->u.d.d);
	  case dr_jmp_bgef:  return ( src1->u.f.f) >= ( src2->u.f.f);
	  case dr_jmp_bgti:  return ( src1->u.i.i) > ( src2->u.i.i);
	  case dr_jmp_bgtu:  return ( src1->u.u.u) > ( src2->u.u.u);
	  case dr_jmp_bgtul:  return ( src1->u.ul.ul) > ( src2->u.ul.ul);
	  case dr_jmp_bgtl:  return ( src1->u.l.l) > ( src2->u.l.l);
	  case dr_jmp_bgtp:  return ((long) src1->u.p.p) > ((long) src2->u.p.p);
	  case dr_jmp_bgtd:  return ( src1->u.d.d) > ( src2->u.d.d);
	  case dr_jmp_bgtf:  return ( src1->u.f.f) > ( src2->u.f.f);
	  case dr_jmp_blei:  return ( src1->u.i.i) <= ( src2->u.i.i);
	  case dr_jmp_bleu:  return ( src1->u.u.u) <= ( src2->u.u.u);
	  case dr_jmp_bleul:  return ( src1->u.ul.ul) <= ( src2->u.ul.ul);
	  case dr_jmp_blel:  return ( src1->u.l.l) <= ( src2->u.l.l);
	  case dr_jmp_blep:  return ((long) src1->u.p.p) <= ((long) src2->u.p.p);
	  case dr_jmp_bled:  return ( src1->u.d.d) <= ( src2->u.d.d);
	  case dr_jmp_blef:  return ( src1->u.f.f) <= ( src2->u.f.f);
	  case dr_jmp_blti:  return ( src1->u.i.i) < ( src2->u.i.i);
	  case dr_jmp_bltu:  return ( src1->u.u.u) < ( src2->u.u.u);
	  case dr_jmp_bltul:  return ( src1->u.ul.ul) < ( src2->u.ul.ul);
	  case dr_jmp_bltl:  return ( src1->u.l.l) < ( src2->u.l.l);
	  case dr_jmp_bltp:  return ((long) src1->u.p.p) < ((long) src2->u.p.p);
	  case dr_jmp_bltd:  return ( src1->u.d.d) < ( src2->u.d.d);
	  case dr_jmp_bltf:  return ( src1->u.f.f) < ( src2->u.f.f);
	  case dr_jmp_bnei:  return ( src1->u.i.i) != ( src2->u.i.i);
	  case dr_jmp_bneu:  return ( src1->u.u.u) != ( src2->u.u.u);
	  case dr_jmp_bneul:  return ( src1->u.ul.ul) != ( src2->u.ul.ul);
	  case dr_jmp_bnel:  return ( src1->u.l.l) != ( src2->u.l.l);
	  case dr_jmp_bnep:  return ((long) src1->u.p.p) != ((long) src2->u.p.p);
	  case dr_jmp_bned:  return ( src1->u.d.d) != ( src2->u.d.d);
	  case dr_jmp_bnef:  return ( src1->u.f.f) != ( src2->u.f.f);
    }return 0;
}
int emulate_branchi(int code, struct reg_type *src1, long imm)
{
    switch(code) {
	  case dr_jmp_beqi:  return ( src1->u.i.i) ==  imm; break;
	  case dr_jmp_bequ:  return ( src1->u.u.u) ==  imm; break;
	  case dr_jmp_bequl:  return ( src1->u.ul.ul) ==  imm; break;
	  case dr_jmp_beql:  return ( src1->u.l.l) ==  imm; break;
	  case dr_jmp_beqp:  return ((char*) src1->u.p.p) == (char*) imm; break;
	  case dr_jmp_bgei:  return ( src1->u.i.i) >=  imm; break;
	  case dr_jmp_bgeu:  return ( src1->u.u.u) >=  imm; break;
	  case dr_jmp_bgeul:  return ( src1->u.ul.ul) >=  imm; break;
	  case dr_jmp_bgel:  return ( src1->u.l.l) >=  imm; break;
	  case dr_jmp_bgep:  return ((char*) src1->u.p.p) >= (char*) imm; break;
	  case dr_jmp_bgti:  return ( src1->u.i.i) >  imm; break;
	  case dr_jmp_bgtu:  return ( src1->u.u.u) >  imm; break;
	  case dr_jmp_bgtul:  return ( src1->u.ul.ul) >  imm; break;
	  case dr_jmp_bgtl:  return ( src1->u.l.l) >  imm; break;
	  case dr_jmp_bgtp:  return ((char*) src1->u.p.p) > (char*) imm; break;
	  case dr_jmp_blei:  return ( src1->u.i.i) <=  imm; break;
	  case dr_jmp_bleu:  return ( src1->u.u.u) <=  imm; break;
	  case dr_jmp_bleul:  return ( src1->u.ul.ul) <=  imm; break;
	  case dr_jmp_blel:  return ( src1->u.l.l) <=  imm; break;
	  case dr_jmp_blep:  return ((char*) src1->u.p.p) <= (char*) imm; break;
	  case dr_jmp_blti:  return ( src1->u.i.i) <  imm; break;
	  case dr_jmp_bltu:  return ( src1->u.u.u) <  imm; break;
	  case dr_jmp_bltul:  return ( src1->u.ul.ul) <  imm; break;
	  case dr_jmp_bltl:  return ( src1->u.l.l) <  imm; break;
	  case dr_jmp_bltp:  return ((char*) src1->u.p.p) < (char*) imm; break;
	  case dr_jmp_bnei:  return ( src1->u.i.i) !=  imm; break;
	  case dr_jmp_bneu:  return ( src1->u.u.u) !=  imm; break;
	  case dr_jmp_bneul:  return ( src1->u.ul.ul) !=  imm; break;
	  case dr_jmp_bnel:  return ( src1->u.l.l) !=  imm; break;
	  case dr_jmp_bnep:  return ((char*) src1->u.p.p) != (char*) imm; break;
    }return 0;
}
#define CONV(x,y) ((x<<4)+y)
void emulate_convert(int code, struct reg_type *dest, struct reg_type *src)
{
    switch(code) {
	case CONV(DR_C, DR_I): dest->u.i.i = ((char)(0xff & src->u.l.l)); break;
	case CONV(DR_C, DR_U): dest->u.u.u = ((char)(0xff & src->u.l.l)); break;
	case CONV(DR_C, DR_UL): dest->u.ul.ul = ((char)(0xff & src->u.l.l)); break;
	case CONV(DR_C, DR_L): dest->u.l.l = ((char)(0xff & src->u.l.l)); break;
	case CONV(DR_D, DR_I): dest->u.i.i = src->u.d.d; break;
	case CONV(DR_D, DR_U): dest->u.u.u = src->u.d.d; break;
	case CONV(DR_D, DR_UL): dest->u.ul.ul = src->u.d.d; break;
	case CONV(DR_D, DR_L): dest->u.l.l = src->u.d.d; break;
	case CONV(DR_F, DR_I): dest->u.i.i = src->u.f.f; break;
	case CONV(DR_F, DR_U): dest->u.u.u = src->u.f.f; break;
	case CONV(DR_F, DR_UL): dest->u.ul.ul = src->u.f.f; break;
	case CONV(DR_F, DR_L): dest->u.l.l = src->u.f.f; break;
	case CONV(DR_I, DR_U): dest->u.u.u = ((int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_I, DR_UL): dest->u.ul.ul = ((int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_I, DR_L): dest->u.l.l = ((int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_L, DR_I): dest->u.i.i = src->u.l.l; break;
	case CONV(DR_L, DR_U): dest->u.u.u = src->u.l.l; break;
	case CONV(DR_L, DR_UL): dest->u.ul.ul = src->u.l.l; break;
	case CONV(DR_S, DR_I): dest->u.i.i = ((short)(0xffff & src->u.l.l)); break;
	case CONV(DR_S, DR_U): dest->u.u.u = ((short)(0xffff & src->u.l.l)); break;
	case CONV(DR_S, DR_UL): dest->u.ul.ul = ((short)(0xffff & src->u.l.l)); break;
	case CONV(DR_S, DR_L): dest->u.l.l = ((short)(0xffff & src->u.l.l)); break;
	case CONV(DR_U, DR_I): dest->u.i.i = ((unsigned int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_U, DR_UL): dest->u.ul.ul = ((unsigned int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_U, DR_L): dest->u.l.l = ((unsigned int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_UL, DR_I): dest->u.i.i = (long)src->u.ul.ul; break;
	case CONV(DR_UL, DR_U): dest->u.u.u = (long)src->u.ul.ul; break;
	case CONV(DR_UL, DR_L): dest->u.l.l = (long)src->u.ul.ul; break;
	case CONV(DR_US, DR_I): dest->u.i.i = ((unsigned short)(0xffff & src->u.l.l)); break;
	case CONV(DR_US, DR_U): dest->u.u.u = ((unsigned short)(0xffff & src->u.l.l)); break;
	case CONV(DR_US, DR_UL): dest->u.ul.ul = ((unsigned short)(0xffff & src->u.l.l)); break;
	case CONV(DR_US, DR_L): dest->u.l.l = ((unsigned short)(0xffff & src->u.l.l)); break;
	case CONV(DR_UC, DR_I): dest->u.i.i = ((unsigned char)(0xff & src->u.l.l)); break;
	case CONV(DR_UC, DR_U): dest->u.u.u = ((unsigned char)(0xff & src->u.l.l)); break;
	case CONV(DR_UC, DR_UL): dest->u.ul.ul = ((unsigned char)(0xff & src->u.l.l)); break;
	case CONV(DR_UC, DR_L): dest->u.l.l = ((unsigned char)(0xff & src->u.l.l)); break;
	case CONV(DR_D, DR_F): dest->u.f.f = src->u.d.d; break;
	case CONV(DR_F, DR_D): dest->u.d.d = src->u.f.f; break;
	case CONV(DR_I, DR_F): dest->u.f.f = ((int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_I, DR_D): dest->u.d.d = ((int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_L, DR_F): dest->u.f.f = src->u.l.l; break;
	case CONV(DR_L, DR_D): dest->u.d.d = src->u.l.l; break;
	case CONV(DR_U, DR_F): dest->u.f.f = ((unsigned int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_U, DR_D): dest->u.d.d = ((unsigned int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_UL, DR_F): dest->u.f.f = (long)src->u.ul.ul; break;
	case CONV(DR_UL, DR_D): dest->u.d.d = (long)src->u.ul.ul; break;
	case CONV(DR_I, DR_C): dest->u.c.c = ((int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_I, DR_S): dest->u.s.s = ((int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_L, DR_C): dest->u.c.c = src->u.l.l; break;
	case CONV(DR_L, DR_S): dest->u.s.s = src->u.l.l; break;
	case CONV(DR_U, DR_C): dest->u.c.c = ((unsigned int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_U, DR_S): dest->u.s.s = ((unsigned int)(0xffffffff & src->u.l.l)); break;
	case CONV(DR_UL, DR_C): dest->u.c.c = (long)src->u.ul.ul; break;
	case CONV(DR_UL, DR_S): dest->u.s.s = (long)src->u.ul.ul; break;
	case CONV(DR_P, DR_UL): dest->u.ul.ul = src->u.l.l; break;
	case CONV(DR_UL, DR_P): dest->u.p.p = (long)src->u.ul.ul; break;
        default: printf("convert missed case %lx \n", code); break;}
}
int emulate_loadi(int code, struct reg_type *dest, struct reg_type *src, long imm)
{
    switch(code) {
	case DR_C: dest->u.l.l = (long) *((char*)(src->u.l.l + imm)); break;
	case DR_D: dest->u.d.d = *((double*)(src->u.l.l + imm)); break;
	case DR_F: dest->u.f.f = *((float*)(src->u.l.l + imm)); break;
	case DR_I: dest->u.l.l = (long) *((int*)(src->u.l.l + imm)); break;
	case DR_L: dest->u.l.l = (long) *((long*)(src->u.l.l + imm)); break;
	case DR_S: dest->u.l.l = (long) *((short*)(src->u.l.l + imm)); break;
	case DR_U: dest->u.l.l = (long) *((unsigned int *)(src->u.l.l + imm)); break;
	case DR_UL: dest->u.l.l = (long) *((unsigned long *)(src->u.l.l + imm)); break;
	case DR_US: dest->u.l.l = (long) *((unsigned short *)(src->u.l.l + imm)); break;
	case DR_UC: dest->u.l.l = (long) *((unsigned char*)(src->u.l.l + imm)); break;
    }return 0;
}
int emulate_storei(int code, struct reg_type *dest, struct reg_type *src, long imm)
{
    switch(code) {
	case DR_C: *((char*)(src->u.l.l + imm)) = dest->u.c.c; break;
	case DR_D: *((double*)(src->u.l.l + imm)) = dest->u.d.d; break;
	case DR_F: *((float*)(src->u.l.l + imm)) = dest->u.f.f; break;
	case DR_I: *((int*)(src->u.l.l + imm)) = dest->u.i.i; break;
	case DR_L: *((long*)(src->u.l.l + imm)) = dest->u.l.l; break;
	case DR_S: *((short*)(src->u.l.l + imm)) = dest->u.s.s; break;
	case DR_U: *((unsigned int *)(src->u.l.l + imm)) = dest->u.u.u; break;
	case DR_UL: *((unsigned long *)(src->u.l.l + imm)) = dest->u.ul.ul; break;
	case DR_US: *((unsigned short *)(src->u.l.l + imm)) = dest->u.us.us; break;
	case DR_UC: *((unsigned char*)(src->u.l.l + imm)) = dest->u.uc.uc; break;
    }return 0;
}
