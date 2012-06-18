AC_PREREQ([2.49])
AC_INIT([liblacewing], [0.3.0], [])

AC_PROG_CXX

AC_CONFIG_MACRO_DIR([m4])

AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],
		 [Build a debug version of the library @<:@default=disabled@:>@])],
	[enable_debug="$enableval"],
	[enable_debug=no]
)

AC_ARG_ENABLE([i486],
	[AS_HELP_STRING([--enable-i486],
		 [Tell the compiler to build for i486 @<:@default=disabled@:>@])],
	[enable_i486="$enableval"],
	[enable_i486=no]
)

CFLAGS="$CFLAGS -DLacewingLibrary -DHAVE_CONFIG_H -fvisibility=hidden"

if test "x$enable_debug" = xyes; then
	CFLAGS="$CFLAGS -DPOSIX_SOURCE -DDEBUG -DLacewingDebug -DHTTP_PARSER_STRICT=1 -DHTTP_PARSER_DEBUG=1 -g -O0"
else
	CFLAGS="$CFLAGS -DPOSIX_SOURCE -DHTTP_PARSER_STRICT=0 -DHTTP_PARSER_DEBUG=0 -O2"
fi

if test "x$enable_i486" = xyes; then
	CFLAGS="$CFLAGS -march=i486"
fi

SO_EXT="so"

OBJECTS="build/Global.o build/Sync.o build/Filter.o build/Address.o build/StreamGraph.o \
    build/Stream.o build/Error.o build/Webserver.o build/HTTP.o build/Multipart.o build/http-parser.o \
    build/MimeTypes.o build/Request.o build/Sessions.o build/Pipe.o \
    build/addr_flat.o build/eventpump_flat.o build/global_flat.o build/server_flat.o build/timer_flat.o \
    build/webserver_flat.o build/filter_flat.o build/udp_flat.o build/client_flat.o \
    build/error_flat.o build/sync_flat.o build/thread_flat.o build/FlashPolicy.o \
    build/Pump.o build/flashpolicy_flat.o build/stream_flat.o build/fdstream_flat.o \
    build/file_flat.o build/pipe_flat.o build/pump_flat.o"

COMMONDEPS="include/Lacewing.h src/Common.h src/Backlog.h src/HeapBuffer.h \
    src/TimeHelper.h src/Utility.h src/webserver/Common.h src/webserver/Map.h \
    src/webserver/http/HTTP.h src/Address.h src/Stream.h"

if test "x$EXEEXT" = "x.exe"; then

    OBJECTS="$OBJECTS build/windows-Client.o build/windows-Compat.o build/windows-Event.o build/windows-EventPump.o \
        build/windows-Server.o build/windows-Thread.o build/windows-Timer.o build/windows-UDP.o \
        build/windows-FDStream.o build/windows-File.o build/windows-WinSSLClient.o"

    COMMONDEPS="$COMMONDEPS src/windows/Compat.h"

    SO_EXT="dll"
    LIBS="$LIBS -lmswsock -lws2_32 -lcrypt32 -lsecur32"

else

	CFLAGS="$CFLAGS -fPIC"

	OBJECTS="$OBJECTS build/unix-Client.o build/unix-Event.o build/unix-EventPump.o \
        build/unix-Server.o build/unix-Thread.o build/unix-Timer.o build/unix-UDP.o \
        build/unix-FDStream.o build/unix-File.o build/openssl-SSLClient.o"

	COMMONDEPS="$COMMONDEPS src/unix/SendFile.h src/unix/EventPump.h"

	AC_CHECK_HEADERS([malloc.h sys/epoll.h sys/prctl.h sys/sendfile.h \
						  netdb.h openssl/md5.h sys/timerfd.h])

	AC_CHECK_HEADERS([CoreServices/CoreServices.h], [[LIBS="$LIBS -framework CoreServices "]
		[SO_EXT="dylib"]])

	AC_CHECK_FUNCS([timegm kqueue vasprintf])

	AC_CHECK_LIB([ssl], [SSL_library_init])
	AC_CHECK_LIB([pthread], [pthread_create])
	AC_CHECK_LIB([rt], [clock_gettime])

	AC_SEARCH_LIBS([MD5_Init], [ssl crypto])

	AC_CHECK_FUNCS([clock_gettime])

	AC_CHECK_DECLS([PR_SET_NAME], [], [], [AC_INCLUDES_DEFAULT
	[#include <sys/prctl.h>]])

	AC_CHECK_DECLS([TCP_CORK, TCP_NOPUSH, MSG_NOSIGNAL, SO_NOSIGPIPE], [], [], [AC_INCLUDES_DEFAULT
	[#include <sys/socket.h>
	 #include <netinet/tcp.h>]])

	AC_CHECK_DECLS([EPOLLRDHUP], [], [], [AC_INCLUDES_DEFAULT
	[#include <sys/epoll.h>]])

	AC_CHECK_DECLS([SSL_MODE_RELEASE_BUFFERS], [], [], [AC_INCLUDES_DEFAULT
	[#include <openssl/ssl.h>]])

fi

CXXFLAGS="$CFLAGS -fno-exceptions -fno-rtti"

AC_SUBST(OBJECTS, $OBJECTS)
AC_SUBST(COMMONDEPS, $COMMONDEPS)

SO_NAME=liblacewing.$SO_EXT.0.3

AC_SUBST(SO_NAME, $SO_NAME)
AC_SUBST(SO_EXT, $SO_EXT)

if test $SO_EXT = "so"; then
	AC_SUBST(SONAME_FLAGS, [-Wl,-soname,$SO_NAME])
else
	AC_SUBST(SONAME_FLAGS, [  ])
fi

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

