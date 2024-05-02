INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

bin_PROGRAMS = 
lib_LTLIBRARIES = 

lib_LTLIBRARIES += libptl_uuid.la

libptl_uuid_la_SOURCES = uuid.c 
libptl_uuid_la_SOURCES += uuid_dce.c
libptl_uuid_la_SOURCES += uuid_mac.c
libptl_uuid_la_SOURCES += uuid_md5.c
libptl_uuid_la_SOURCES += uuid_prng.c
libptl_uuid_la_SOURCES += uuid_sha1.c
libptl_uuid_la_SOURCES += uuid_str.c
libptl_uuid_la_SOURCES += uuid_ui128.c
libptl_uuid_la_SOURCES += uuid_ui64.c

bin_PROGRAMS += ptl_uuid

ptl_uuid_SOURCES = uuid_cli.c
ptl_uuid_LDFLAGS = $(PORTALS_LDFLAGS)
ptl_uuid_LDADD = libptl_uuid.la
ptl_uuid_LDADD += $(PORTALS_LIBS)

CLEANFILES = *~ core.* *.loT

