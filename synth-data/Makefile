EXEC	  = gen-mfr-inst gen-occ-inst

SOURCES   = \
	BitVec.ml	\
	FArray.ml	\
	Graph.ml

INCLUDES  =
OCAMLC    = ocamlc -w A -warn-error A $(INCLUDES)
OCAMLOPT  = ocamlopt -warn-error A $(INCLUDES)
OCAMLDEP  = ocamldep $(INCLUDES)
#PROFILE  = -p

OBJS	  = $(SOURCES:.ml=.cmx)

all: depend $(EXEC)

gen-mfr-inst: $(OBJS) gen-mfr-inst.cmx
	$(OCAMLOPT) $(PROFILE) -o $@ $(LIBS) $^

gen-occ-inst: $(OBJS) gen-occ-inst.cmx
	$(OCAMLOPT) $(PROFILE) -o $@ $(LIBS) $^

clean:
	rm -f *.cm[iox] *.o $(EXEC) core gmon.out

realclean: clean
	rm -f *~ *.old *.bak

.SUFFIXES: .ml .mli .cmi .cmx


.mli.cmi:
	$(OCAMLC) -c $<

.ml.cmx:
	$(OCAMLOPT) $(PROFILE) -c $<

.depend: depend
depend: $(SOURCES)
	$(OCAMLDEP) *.mli *.ml > .depend

include .depend
