#
#
#

include $(top_srcdir)/Makecore.am

AM_CPPFLAGS += -I../mg/media -I$(top_srcdir)/src/mg/media -I$(top_srcdir)/src/b64/include -DBASE64=1

bin_PROGRAMS = mg

mg_SOURCES =  mp4info.cpp

mg_LDADD =  $(top_builddir)/src/mg/core/libmgcore.a \
            $(top_builddir)/src/mg/media/libmgmedia.a \
	    $(top_builddir)/src/b64/libbase64.a



#MOSTLYCLEANFILES = $(CLEAN_GCOV) $(TESTS) 
