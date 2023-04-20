# CC = g++

CSRC += exprRead.c
HEADERS += expr.h exprInt.h
LEXSRC += expr.l
YACCSRC += expr.y
GENERATEDCSRC += exprYacc.c 

$(objectdir)/exprYacc.c: expr.y expr.h exprInt.h exprLex.c
	$(YACC) -pExprYy -t -o $(objectdir)/exprYacc.c $<
	-@chmod 0664 $(objectdir)/exprYacc.c

exprLex.c: expr.l expr.h exprInt.h
	$(LEX) -PExprYy -o$(objectdir)/exprLex.c $<
	-@chmod 0664 $(objectdir)/exprLex.c

#$(objectdir)/exprYacc.o: $(objectdir)/exprYacc.c
#	umask 2 ; $(CC) -c $(ALLCFLAGS) $(INCLUDEDIRS) -o $@ $<
$(objectdir)/exprYacc.o: $(objectdir)/exprYacc.c
	umask 2 ; gcc -c $(ALLCFLAGS) $(INCLUDEDIRS) -o $@ $<


DEPENDENCYFILES = $(CSRC)
