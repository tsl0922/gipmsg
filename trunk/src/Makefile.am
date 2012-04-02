AM_CFLAGS = $(GIPMSG_CFLAGS) -DDEBUG

bin_PROGRAMS = gipmsg

noinst_LTLIBRARIES = libgipmsg.la

gipmsg_SOURCES = \
	mainwin.c mainwin.h \
	senddlg.c senddlg.h \
	emotion.c emotion.h \
	main.c

gipmsg_LDADD = libgipmsg.la $(GIPMSG_LIBS)

libgipmsg_la_SOURCES = \
	os-support.h ipmsg.h \
	ipmsg-types.h list.h \
	common.h \
	debug.c debug.h \
	util.c util.h \
	myconfig.c myconfig.h \
	message.c message.h \
	core.c core.h \
	user.c user.h \
	sendfile.c sendfile.h