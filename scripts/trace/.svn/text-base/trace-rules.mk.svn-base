# This should be included into a Makefile.am file


if HAVE_PABLO

SUFFIXES = .sddf .xml .gnu

.sddf.xml: $<
	sddf2xml -o .tmp.xml $<
	xsltproc $(top_srcdir)/scripts/trace/sddf2lwfs.xsl .tmp.xml > $@

.xml.gnu-interval: $<
	$(top_srcdir)/scripts/trace/gen-gnu-interval.pl --scriptdir=$(top_srcdir)/scripts/trace --src=$< --dest=$@ --eps=$*.eps

.xml.gnu-count: $<
	$(top_srcdir)/scripts/trace/gen-gnu-count.pl --scriptdir=$(top_srcdir)/scripts/trace --src=$< --dest=$@ --eps=$*.eps

endif
