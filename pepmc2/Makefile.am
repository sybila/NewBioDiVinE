CXX=g++

include $(top_srcdir)/Makefile.am.global

bin_PROGRAMS = pepmc
pepmc_SOURCES = main.cpp


pepmc: force
	$(CXX) -std=c++11 -o $@ $(pepmc_SOURCES) -DPEPMC_LINUX_SETAFFINITY  -lhoard  -lboost_thread -lboost_system -lpthread 
	mv $@ $(top_srcdir)/bin/divine.pepmc

force: ;

clean:
	rm -f $(top_srcdir)/bin/divine.pepmc

