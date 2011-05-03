
bin_PROGRAMS = zipbrowser

zipbrowser_SOURCES = 	\
	    download.c	\
	    ipc.c	\
	    main.c	\
	    menu.c	\
	    metadata.c	\
	    view.c      \
	    ioapi.c     \
	    unzip.c

AM_CFLAGS = -Wall -Werror -Wextra -Wno-unused-parameter	\
	    -DDATADIR=\"$(pkgdatadir)\"		\
	    -DSYSCONFDIR=\"$(sysconfdir)\"	\
	    -I$(top_srcdir)/include

AM_CPPFLAGS = $(DEPS_CFLAGS) $(LIBERXX_CFLAGS) $(HTTPD_CFLAGS)
AM_LDFLAGS  = $(DEPS_LIBS)   $(LIBERXX_LIBS) $(HTTPD_LIBS) -lstdc++

