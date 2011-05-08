ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

lib_LTLIBRARIES = liblacewing.la

liblacewing_la_LIBADD = -lpthread -lssl
liblacewing_la_LDFLAGS = -version-info 1:0:0

liblacewing_la_SOURCES = \
    src/Global.cc \
    src/Sync.cc \
    src/SpinSync.cc \
    src/Filter.cc \
    src/Address.cc \
    src/Error.cc \
    src/RelayServer.cc \
    src/webserver/Webserver.cc \
    src/webserver/Webserver.Incoming.cc \
    src/webserver/Webserver.Outgoing.cc \
    src/webserver/Webserver.Sessions.cc \
    src/webserver/Webserver.Multipart.cc \
    src/webserver/Webserver.MimeTypes.cc \
    src/unix/Event.cc \
    src/unix/MD5Hasher.cc \
    src/unix/EventPump.cc \
    src/unix/Server.cc \
    src/unix/Timer.cc \
    src/unix/UDP.cc \
    src/unix/Client.cc \
    src/c/addr_flat.cc \
    src/c/eventpump_flat.cc \
    src/c/global_flat.cc \
    src/c/server_flat.cc \
    src/c/timer_flat.cc \
    src/c/webserver_flat.cc \
    src/c/filter_flat.cc \
    src/c/udp_flat.cc \
    src/c/client_flat.cc \
    src/c/error_flat.cc \
    src/c/sync_flat.cc \
    src/c/ssync_flat.cc
    
library_includedir = $(includedir)
library_include_HEADERS = ./include/Lacewing.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lacewing.pc

dist_noinst_SCRIPTS = autogen.sh

