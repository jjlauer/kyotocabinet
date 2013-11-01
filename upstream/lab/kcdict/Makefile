COMMANDFILES = kcdictmgr
UTILFILES = kcdictwntotsv kcdictejrtotsv
CGIFILES = kcdictsearch.cgi

prefix = /usr/local
exec_prefix = ${prefix}
BINDIR = ${exec_prefix}/bin
LIBEXECDIR = ${exec_prefix}/libexec

CXX = g++
CPPFLAGS = -I. -I.. -I../.. -I/usr/local/include -D_GNU_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D__EXTENSIONS__
CXXFLAGS = -O2 -Wall -fPIC -fsigned-char
LDFLAGS = -L. -L.. -L../.. -L/usr/local/lib -Wl,-rpath-link,.:..:../..:/usr/local/lib:.:/usr/local/lib:/usr/local/lib:. -Wl,--as-needed
LIBS = -lkyotocabinet
#LIBS = -static -lkyotocabinet -lz -lstdc++ -lrt -lpthread -lm -lc
RUNENV = LD_LIBRARY_PATH=.:/usr/local/lib:/usr/local/lib:.:..:../..


all : $(COMMANDFILES)


install :
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -Rf $(COMMANDFILES) $(UTILFILES) $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(LIBEXECDIR)
	cp -Rf $(CGIFILES) $(DESTDIR)$(LIBEXECDIR)


install-strip :
	$(MAKE) DESTDIR=$(DESTDIR) install
	cd $(DESTDIR)$(BINDIR) && strip $(COMMANDFILES)


uninstall :
	-cd $(DESTDIR)$(BINDIR) && rm -f $(COMMANDFILES) $(UTILFILES)
	-cd $(DESTDIR)$(LIBEXECDIR) && rm -f $(CGIFILES)


clean :
	rm -rf $(COMMANDFILES) *.exe *.o a.out check.out gmon.out leak.log \
	  casket* *.kct wordnet.tsv eijiro-*.tsv *~


check :
	rm -rf dict.kct
	$(RUNENV) ./kcdictmgr import dict.kct sample.tsv
	$(RUNENV) ./kcdictmgr search dict.kct 't' > check.out
	$(RUNENV) ./kcdictmgr search dict.kct 'aaa' > check.out
	$(RUNENV) ./kcdictmgr search -max 3 dict.kct '' > check.out
	$(RUNENV) ./kcdictmgr search -f dict.kct 'tokyo' > check.out
	$(RUNENV) ./kcdictmgr search -a dict.kct 'tok' > check.out
	$(RUNENV) ./kcdictmgr search -m dict.kct 'e' > check.out
	$(RUNENV) ./kcdictmgr search -r dict.kct '^f' > check.out
	$(RUNENV) ./kcdictmgr search -tm dict.kct 'e' > check.out
	$(RUNENV) ./kcdictmgr search -tr dict.kct 'r' > check.out
	rm -rf dict.kct


dict :
	ls *.tsv | while read path ; \
	  do \
	    $(RUNENV) ./kcdictmgr import "$${path%.tsv}.kct" "$$path" ; \
	  done


.SUFFIXES :
.SUFFIXES : .cc .o

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

.cc.o :
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<


kcdictmgr : kcdictmgr.o
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)
