SUBDIRS = . test_n1 test_n2

XMLEXTRA = \
	test_phys_client.xml \
	test_phys_server.xml \
	test_threads.xml \
	test_atomic_mixed.xml \
	test_basic_rt.xml \
	test_loopback_move.xml

EXTRA_DIST = $(XMLEXTRA) make_test_relay.pl

noinst_PROGRAMS = ptl_test make_test_atomic make_test_swap make_test_move test

# Each binary needs the portals library and the runtime
PORTALSLIB = $(top_builddir)/src/libportals.la $(top_builddir)/test/libtestsupport.la
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/test $(XML_CPPFLAGS)

ptl_test_LDFLAGS = -L$(top_builddir)/src $(LIBTOOL_WRAPPER_LDFLAGS)
ptl_test_LDADD =  $(PORTALSLIB) -lpthread $(XML_LIBS)
ptl_test_SOURCES = \
	api.c \
	api.h \
	cio.c \
	cio.h \
	data.h \
	dict.c \
	dict.h \
	enum.c \
	main.c \
	mask.c \
	ptl_test.h \
	rt.c \
	run.c

test_LDFLAGS = -L$(top_builddir)/src
test_LDADD = $(PORTALSLIB) -lpthread
test_SOURCES = \
	test.c

make_test_move_SOURCES = data.c make_test_move.c
make_test_swap_SOURCES = data.c make_test_swap.c
make_test_atomic_SOURCES = data.c make_test_atomic.c

CLEANFILES = runxml1.sh runxml2.sh

