#
#  This file, unix_to_nt.sed is used to create configure.nt from configure.
#  It performs some basic substitutions that makes the configure script work
#  (kind of) at least under the MKS toolkit version of ksh.
#
s=/dev/null=NUL:=g
s/ac_exeext=/ac_exeext=.exe/g
s/ac_objext=o/ac_objext=obj/g
s/conftest /conftest.exe /g
s/conftest;/conftest.exe;/g
s=/bin/sh=sh=g
s=/lib/cpp=cpp=g
s=-lvcode=$ac_cv_tcc_cg_vcode_x86_link_dir/vcode.lib=g
s=libvcode.a=vcode.lib=g
s%ac_cv_tcc_cg_vcode_x86_link_arg=.*%ac_cv_tcc_cg_vcode_x86_link_arg=""%g
s=-licode=$ac_cv_tcc_cg_icode_link_dir/icode.lib=g
s=libicode.a=icode.lib=g
s%ac_cv_tcc_cg_icode_link_arg=.*%ac_cv_tcc_cg_icode_link_arg=""%g
s/ac_cv_prog_RANLIB=":"/ac_cv_prog_RANLIB="true"/g
s/ac_cv_prog_LN_S=ln/ac_cv_prog_LN_S=cp/
/[ *]ac_dir/ s#/]\*$%%'`#/]*$%%'|sed 's%^[a-zA-Z]:%%'`#
/^ac_err=/ {i\
ac_err=`grep -v '^ *+' conftest.out | grep -v '^conftest.c$'`
;d
}
/[/$]*) INSTALL="$ac_given_INSTALL" ;;/ {i\
  ?:[/$]*) INSTALL="$ac_given_INSTALL" ;;
}

