AC_PREREQ([2.49])
AC_INIT([liblacewing], [0.4.2], [])

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

AC_ARG_ENABLE([spdy],
	[AS_HELP_STRING([--enable-spdy],
		 [Enable SPDY support @<:@default=enabled@:>@])],
	[enable_spdy="$enableval"],
	[enable_spdy=yes]
)

CFLAGS="$CFLAGS -DLacewingLibrary -DHAVE_CONFIG_H -fvisibility=hidden"

if test "x$enable_debug" = xyes; then
	CFLAGS="$CFLAGS -DDEBUG -DLacewingDebug -DHTTP_PARSER_STRICT=1 -DHTTP_PARSER_DEBUG=1 -g -O0"
else
	CFLAGS="$CFLAGS -DHTTP_PARSER_STRICT=0 -DHTTP_PARSER_DEBUG=0 -O2"
fi

if test "x$enable_i486" = xyes; then
	CFLAGS="$CFLAGS -march=i486"
fi

SO_EXT="so"

if test "x$enable_spdy" = xyes; then

    OBJECTS="$OBJECTS build/spdy_bytes.o build/spdy_control.o \
             build/spdy_credential.o build/spdy_ctx.o build/spdy_goaway.o \
             build/spdy_headers.o build/spdy_nv_block.o build/spdy_ping.o \
             build/spdy_rst_stream.o build/spdy_settings.o build/spdy_stream.o \
             build/spdy_strings.o build/spdy_syn_reply.o build/spdy_syn_stream.o \
             build/spdy_zlib.o build/spdy_window_update.o build/SPDY.o"

	AC_CHECK_LIB([z], [inflate])

else
    CFLAGS="$CFLAGS -DLacewingNoSPDY"
fi

if test "x$EXEEXT" = "x.exe"; then

    OBJECTS="$OBJECTS build/windows-Client.o build/windows-lw_compat.o \
             build/windows-Event.o build/windows-EventPump.o \
             build/windows-Server.o build/windows-Thread.o \
             build/windows-Timer.o build/windows-UDP.o \
             build/windows-FDStream.o build/windows-File.o \
             build/windows-WinSSLClient.o build/windows-Sync.o \
             build/windows-lw_global.o"

    COMMONDEPS="$COMMONDEPS src/windows/lw_compat.h src/windows/lw_common.h"

    SO_EXT="dll"
    LIBS="$LIBS -lmswsock -lws2_32 -lcrypt32 -lsecur32"

else

	CFLAGS="$CFLAGS -fPIC"

	OBJECTS="$OBJECTS build/unix-Client.o build/unix-Event.o build/unix-EventPump.o \
        build/unix-Server.o build/unix-Thread.o build/unix-Timer.o build/unix-UDP.o \
        build/unix-FDStream.o build/unix-File.o build/openssl-SSLClient.o \
        build/unix-Sync.o build/unix-lw_global.o"

	COMMONDEPS="$COMMONDEPS src/unix/EventPump.h src/unix/lw_common.h"

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
fi

CXXFLAGS="$CFLAGS -fno-exceptions -fno-rtti"
CFLAGS="-ansi -Wall -Wno-format $CFLAGS"

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

