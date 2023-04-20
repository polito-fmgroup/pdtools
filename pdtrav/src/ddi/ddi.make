CSRC += ddiAig.c ddiAiger.c ddiArray.c ddiExpr.c ddiExist.c ddiMgr.c ddiVar.c ddiVarsetarray.c ddiBdd.c ddiGeneric.c ddiNew.c ddiVararray.c ddiBddarray.c ddiMeta.c ddiUtil.c ddiVarset.c ddiAbcLink.c ddiSat.c ddiLemma.c ddiSim.c data.c cluster.c
HEADERS += ddi.h ddiInt.h data.h cluster.h
 
$(objectdir)/ddiAbcLink.o: ddiAbcLink.c
	umask 2 ; gcc -c $(ALLCFLAGS) $(INCLUDEDIRS) -o $@ $<

$(objectdir)/ddiAiger.o: ./ddiAiger.c
	umask 2 ; g++ -x c -c $(ALLCFLAGS) $(INCLUDEDIRS) -o $@ $<

#$(objectdir)/cluster.o: ./cluster.c
#	umask 2 ; g++ -x c -c $(ALLCFLAGS) $(INCLUDEDIRS) -o $@ $<

#$(objectdir)/data.o: ./data.c
#	umask 2 ; g++ -x c -c $(ALLCFLAGS) $(INCLUDEDIRS) -o $@ $<

DEPENDENCYFILES = $(CSRC) 
