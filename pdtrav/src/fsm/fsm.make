CSRC += fsmBlifStore.c fsmMgr.c fsmPortBnet.c fsmProMac.c fsmStore.c fsmUtil.c fsmLoad.c fsmPort.c fsmPortNtr.c fsmRetime.c fsmToken.c fsmPortAiger.c fsmXsim.c fsmFsm.c

HEADERS += fsm.h fsmInt.h aiger.h

$(objectdir)/fsmPortAiger.o: ./fsmPortAiger.c
	umask 2 ; g++ -x c -c $(ALLCFLAGS) $(INCLUDEDIRS) -o $@ $<

 
DEPENDENCYFILES = $(CSRC) 
