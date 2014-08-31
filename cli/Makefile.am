CXX=g++

include $(top_srcdir)/Makefile.am.global

bin_PROGRAMS = newbiodivine
newbiodivine_SOURCES = cli.cpp


newbiodivine: force
	$(CXX) $(CXXFLAGS) -o $@ $(newbiodivine_SOURCES)
	mv $@ $(top_srcdir)/bin/

force: ;

clean:
	rm -f $(top_srcdir)/bin/newbiodivine

