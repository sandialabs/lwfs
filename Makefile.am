# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

SUBDIRS = 

SUBDIRS += src 


if BUILD_DOCS
SUBDIRS += doc
endif


test:
	echo LWFS_ROOT = $(LWFS_ROOT)
	echo DIRS = $(DIRS)
	echo SUBDIRS = $(SUBDIRS)

CLEANFILES = *~
