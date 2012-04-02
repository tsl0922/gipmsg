AM_CFLAGS = -I$(top_srcdir)/src $(GIPMSG_CFLAGS) -DDEBUG

noinst_PROGRAMS = $(TESTS)
progs_ldadd     = $(top_builddir)/src/libgipmsg.la $(GIPMSG_LIBS)

TEST_PROGS			= message
message_SOURCES		= message.c
message_LDADD		= $(progs_ldadd)

TEST_PROGS			+= util
util_SOURCES		= util.c
util_LDADD		= $(progs_ldadd)

TEST_PROGS			+= core
core_SOURCES		= core.c
core_LDADD		= $(progs_ldadd)

TESTS = $(TEST_PROGS)