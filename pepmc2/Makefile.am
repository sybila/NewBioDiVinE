CXX=g++

include $(top_srcdir)/Makefile.am.global

bin_PROGRAMS = pepmc
pepmc_SOURCES = main.cpp


if BUILD_OS_IS_DARWIN
ba=-lboost_thread-mt
else
ba=-DPEPMC_LINUX_SETAFFINITY -lboost_thread
endif


pepmc: force
	$(CXX) -std=c++11 -o $@ $(pepmc_SOURCES) $(ba) -lboost_system -lpthread 
	mv $@ $(top_srcdir)/bin/divine.pepmc

force: ;

clean:
	rm -f $(top_srcdir)/bin/divine.pepmc

