CXX=g++

include $(top_srcdir)/Makefile.am.global

bin_PROGRAMS = pepmc
pepmc_SOURCES = main.cpp


if BUILD_OS_IS_DARWIN
ba=-L$(BOOST_LIBRARY_DIR) -lboost_thread-mt -lboost_system-mt
else
ba=-DPEPMC_LINUX_SETAFFINITY -lboost_thread -lboost_system
endif

BOOST_LIBRARY_DIR = $(BOOST_LIB_DIR)


pepmc: force
	$(CXX) -std=c++11 -o $@ $(pepmc_SOURCES) $(ba) -lpthread 
	mv $@ $(top_srcdir)/bin/divine.pepmc

force: ;

clean:
	rm -f $(top_srcdir)/bin/divine.pepmc

