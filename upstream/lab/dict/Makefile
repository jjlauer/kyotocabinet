BINFILES = kcdictmgr

CXX = g++
CPPFLAGS = -I. -I.. -I../.. -I/usr/local/include -D_GNU_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D__EXTENSIONS__
CXXFLAGS = -O2 -Wall -fPIC -fsigned-char
LDFLAGS = -L. -L.. -L../.. -L/usr/local/lib -Wl,-rpath-link,.:..:../..:/usr/local/lib:.:/usr/local/lib:/usr/local/lib:. -Wl,--as-needed
LIBS = -lkyotocabinet


all : $(BINFILES)


clean :
	rm -rf $(BINFILES) *.exe *.o a.out check.out gmon.out leak.log casket* *~


.SUFFIXES :
.SUFFIXES : .cc .o

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

.cc.o :
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<


kcdictmgr : kcdictmgr.o kcdict.o
	$(CXX) $(CXXFLAGS) -o $@ $< kcdict.o $(LDFLAGS) $(LIBS)


kcdictmgr.o kcdict.o : kcdict.h
