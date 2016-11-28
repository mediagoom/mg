#
#
#

include $(top_srcdir)/Makecore.am

#include_HEADERS= src/mdcore/mdcore.h src/mdcore/mdbitstream.h src/mdcore/mdthread.h src/mdcore/mdchar.h
#nobase_include_HEADERS=
CLEANFILES =

MOSTLYCLEANFILES = $(CLEAN_GCOV) 

lib_LIBRARIES = libbase64.a

# where to install the headers on the system
libbase64_adir = $(includedir)/mg/src/b64


libbase64_a_HEADERS = include/cencode.h \
		      include/cdecode.h
libbase64_a_SOURCES = src/cencode.cpp \
		      src/cdecode.cpp
			

