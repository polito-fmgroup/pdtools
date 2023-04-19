.SUFFIXES: .smv .flatsmv .aig
.smv.flatsmv:
	rm -f $@; smvflatten $< > $@
.flatsmv.aig:
	rm -f $@; smvtoaig $< $@
all: $(shell ls *.smv|sed -e 's,\(.*\)\.smv$$,\1.aig \1.flatsmv,g')
