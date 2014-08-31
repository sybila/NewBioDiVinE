include $(top_srcdir)/Makefile.am.global

CXX=$(MPICXX)
# Automakes's variable for creating libraries:
lib_LIBRARIES = libsevine.a
if HAVEMPI
lib_LIBRARIES += libdivine.a
endif
# Custom variable to become the prefix of paths:
tsrc=$(top_srcdir)/src

# Paths to sources for sequential part of the design
sequential_part_sources = $(tsrc)/common/reporter.cc \
                          $(tsrc)/common/inttostr.cc \
                          $(tsrc)/common/error.cc \
                          $(tsrc)/common/sysinfo.cc \
                          $(tsrc)/common/hash_function.cc \
			  			  $(tsrc)/common/bitarray.cc \
                      $(tsrc)/system/bio/affine_atomic_propositions.cc \
					  $(tsrc)/system/bio/affine_property.cc \
                      $(tsrc)/system/bio/affine_system.cc \
                      $(tsrc)/system/bio/affine_explicit_system.cc \
                          $(tsrc)/system/transition.cc \
                          $(tsrc)/system/state.cc \
                          $(tsrc)/system/path.cc \
                          $(tsrc)/system/system.cc \
                          $(tsrc)/system/system_trans.cc \
                          $(tsrc)/system/process.cc \
                      $(tsrc)/storage/compressor.cc \
                      $(tsrc)/storage/explicit_storage.cc

# Paths to sources for the distributed part of the design (using MPI)
distributed_part_sources = $(tsrc)/common/distr_reporter.cc \
			  			   $(tsrc)/distributed/message.cc \
                           $(tsrc)/distributed/network.cc \
                           $(tsrc)/distributed/distributed.cc \
                           $(tsrc)/common/logger.cc
libsevine_a_SOURCES = $(sequential_part_sources)

if HAVEMPI
libdivine_a_SOURCES = $(sequential_part_sources) $(distributed_part_sources)
endif

