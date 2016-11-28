#
#
#

include $(top_srcdir)/Makecore.am

#include_HEADERS= src/mdcore/mdcore.h src/mdcore/mdbitstream.h src/mdcore/mdthread.h src/mdcore/mdchar.h
#nobase_include_HEADERS=
CLEANFILES =

MOSTLYCLEANFILES = $(CLEAN_GCOV) 

lib_LIBRARIES = libmgcore.a

# where to install the headers on the system
libmgcore_adir = $(includedir)/mg/core


libmgcore_a_HEADERS = core.h \
		      char.h \
                      bitstream.h \
		      thread.h \
		      uvbase.h \
		      file.h \
		      util.h \
		      ctime.h

libmgcore_a_SOURCES = bitstream.cpp \
                      thread.cpp \
		      uvbase.cpp \
		      file.cpp \
		      nx/util.cpp \
		      nx/ctime.cpp
