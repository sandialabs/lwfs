
noinst_DATA  = ufs_stat.eps 
noinst_DATA += ufs_mkdir.eps 
noinst_DATA += ufs_rmdir.eps 
noinst_DATA += ufs_create.eps
noinst_DATA += ufs_remove.eps 

noinst_DATA += lwfs_lookup.eps
noinst_DATA += lwfs_mkdir.eps
noinst_DATA += lwfs_mkdir_time.eps
noinst_DATA += lwfs_rmdir.eps
noinst_DATA += lwfs_rmdir_time.eps
noinst_DATA += lwfs_file_create.eps
noinst_DATA += lwfs_file_create_time.eps
noinst_DATA += lwfs_file_remove.eps
noinst_DATA += lwfs_file_remove_time.eps
#noinst_DATA += getdir.eps 
#noinst_DATA += create.eps
#noinst_DATA += mkdir.eps
#noinst_DATA += remove.eps
#noinst_DATA += rmdir.eps
#noinst_DATA += getattr.eps
#noinst_DATA += ping.eps 

SUFFIXES = .eps .m .gp 


.eps.pdf:  $<
	echo
	cd $(srcdir); \
	epstopdf $<; 

.gp.eps:  $<
	echo
	cd $(srcdir); \
	gnuplot $<; 

.m.eps:  $<
	echo
	cd $(srcdir); \
	octave $<; 


CLEANFILES = $(srcdir)/*.aux $(srcdir)/*.bbl $(srcdir)/*.blg \
			$(srcdir)/*.log $(srcdir)/*~ \
			$(srcdir)/*.pdf $(srcdir)/*.mpx $(srcdir)/*.1 \
			$(srcdir)/*.dvi \
			${noinst_DATA}

clean: 
	cd $(srcdir); rm -rf $(CLEANFILES);
