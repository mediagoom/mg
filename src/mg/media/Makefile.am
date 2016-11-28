#
#
#

include $(top_srcdir)/Makecore.am

#include_HEADERS= src/mdcore/mdcore.h src/mdcore/mdbitstream.h src/mdcore/mdthread.h src/mdcore/mdchar.h
#nobase_include_HEADERS=
CLEANFILES =

MOSTLYCLEANFILES = $(CLEAN_GCOV) 

lib_LIBRARIES = libmgmedia.a

# where to install the headers on the system
libmgmedia_adir = $(includedir)/mg/media

mp4/%.h mp4/%.cpp: $(srcdir)/mp4/%.fl
	cd "$(srcdir)/mp4"; flavor  -x -l $*.fl;cd -;mv $(srcdir)/mp4/$*.cpp ./mp4;mv $(srcdir)/mp4/$*.h ./mp4 

libmgmedia_a_HEADERS = TBitstream.h 
libmgmedia_a_SOURCES = \
 			mp4/aac.cpp \
			mp4/mpeg4_odf.cpp \
		        mp4/mp4.cpp \
			mp4/h264.cpp \
			mp4/ltcextension.cpp \
		      	mp4/mp4i_ext.cpp \
		        mp4/mp4_fragments.cpp \
			mp4/cenc.cpp \
			mp4edit.cpp \
			mp4dynamic.cpp \
			mp4dynamicinfo.cpp
			

