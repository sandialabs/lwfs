# Last Modified: Lisa Glendenning, 14 Jun 07


include ../Makefile.common


source_dir     := ../../src


compute_includes    := $(source_dir)/common/compute.h
compute_sources     := $(source_dir)/common/compute.c
config_includes     := $(source_dir)/common/config.h
config_sources      := $(source_dir)/common/config.c
link_list_includes  := $(source_dir)/common/link_list.h
link_list_sources   := $(source_dir)/common/link_list.c


.PHONY: all clean

all: compute.o config.o link_list.o

clean:
	rm -f *.o


$(source_dir)/common/config.h: $(source_dir)/common/defs.h
$(source_dir)/common/compute.h: $(source_dir)/common/defs.h
$(source_dir)/common/link_list.h: $(source_dir)/common/defs.h

compute.o: $(compute_includes) $(compute_sources)
	$(CC) $(CPPFLAGS) $(compute_sources)

config.o: $(config_includes) $(config_sources)
	$(CC) $(CPPFLAGS) $(config_sources)

link_list.o: $(link_list_includes) $(link_list_sources)
	$(CC) $(CPPFLAGS) $(link_list_sources)
