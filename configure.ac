AC_PREREQ([2.49])
AC_INIT([liblacewing], [0.2.0], [])

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

CXXFLAGS="$CXXFLAGS -DLacewingLibrary -DHAVE_CONFIG_H -fno-exceptions -fno-rtti -fvisibility=hidden -fPIC"

if test "x$enable_debug" = xyes; then
    CXXFLAGS="$CXXFLAGS -DPOSIX_SOURCE -DDEBUG -DLacewingDebug -g -O0"
else
    CXXFLAGS="$CXXFLAGS -DPOSIX_SOURCE -O2"
fi

if test "x$enable_i486" = xyes; then
    CXXFLAGS="$CXXFLAGS -march=i486"
fi

AC_CHECK_HEADERS([malloc.h sys/epoll.h sys/prctl.h sys/sendfile.h \
            netdb.h CoreServices/CoreServices.h openssl/md5.h sys/timerfd.h])

AC_CHECK_FUNCS([timegm kqueue vasprintf])

AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_FUNCS([clock_gettime])

AC_CHECK_DECLS([PR_SET_NAME], [], [], [AC_INCLUDES_DEFAULT
[#include <sys/prctl.h>]])

AC_CHECK_DECLS([TCP_CORK, TCP_NOPUSH], [], [], [AC_INCLUDES_DEFAULT
[#include <netinet/tcp.h>]])

AC_CHECK_DECLS([EPOLLRDHUP], [], [], [AC_INCLUDES_DEFAULT
[#include <sys/epoll.h>]])

AC_CHECK_DECLS([SSL_MODE_RELEASE_BUFFERS], [], [], [AC_INCLUDES_DEFAULT
[#include <openssl/ssl.h>]])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

