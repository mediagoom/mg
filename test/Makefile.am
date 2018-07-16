#
#
#

include $(top_srcdir)/Makecore.am

AM_CPPFLAGS += -I../src/mg/media -I$(top_srcdir)/src/mg/media

bin_PROGRAMS = mgtest

mgtest_SOURCES =  test_bitstream.cpp \
		 test_thread.cpp \
		 test_resource.cpp \
		 test_buffer.cpp \
		 test_uv.cpp \
		 test_mp4_parse.cpp \
		 test_util.cpp	\
		 test.cpp
mgtest_LDADD =  $(top_builddir)/src/mg/core/libmgcore.a \
		$(top_builddir)/src/mg/media/libmgmedia.a

TOUCH_TEST = BITSTREAM RESOURCES UV THREAD MP4

TESTS = $(TOUCH_TEST) test_hls.py test_dash.py test_wine.py test_mg.py
TEST_EXTENSIONS = .py
PY_LOG_COMPILER = $(PYTHON)
LOG_COMPILER = ./mgtest


$(TESTS):
	touch $@




MOSTLYCLEANFILES = $(CLEAN_GCOV) $(TOUCH_TEST) 

#lib_LTLIBRARIES = libmdcore.la
#libmdcore_la_CFLAGS = @CFLAGS@
#libuv_la_LDFLAGS = -no-undefined -version-info 1:0:0
#libmdcore_la_SOURCES = src/mdcore/mdcore.h \
#		   src/mdcore/mdbitstream.h \
#		   src/mdcore/mdthread.h \
#		   src/mdcore/mdchar.h \
#		   src/mdcore/mdthread.cpp


#EXTRA_DIST = test/fixtures/empty_file \
#             test/fixtures/load_error.node \
#             include 



#TESTS = test/run_tests
#check_PROGRAMS = test/run_tests
#test_run_tests_CFLAGS =

#test_run_tests_SOURCES = test/test_bitstream.cpp \
#			 test/test_thread.cpp \
#			 test/test.cpp
#test_run_tests_LDADD = libuv.la

