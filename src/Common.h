/*
    Copyright (C) 2011 James McLaughlin

    This file is part of Lacewing.

    Lacewing is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lacewing is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Lacewing.  If not, see <http://www.gnu.org/licenses/>.

*/

#define LacewingInternal

#ifdef _MSC_VER

    #ifndef LacewingUnixDevelDummy
        #define LacewingWindows
    #endif

    #ifdef _WIN64
        #define Lacewing64
    #endif

    #ifdef _DEBUG
        #define LacewingDebug
    #endif

    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif

    #ifndef _CRT_NONSTDC_NO_WARNINGS
        #define _CRT_NONSTDC_NO_WARNINGS
    #endif

#else

    #ifdef HAVE_CONFIG_H
        #include "config.h"
    #else
        #error Valid config.h required for non-MSVC! Run ./configure
    #endif

#endif

#ifdef LacewingLibrary
    #ifdef _MSC_VER
        #define LacewingFunction __declspec(dllexport)
    #else
        #ifdef __GNUC__
            #define LacewingFunction __attribute__((visibility("default"))
        #else
            #error "Don't know how to build the library with this compiler"
        #endif
    #endif
#else
    #define LacewingFunction
#endif

inline void LacewingAssert(bool Expression)
{
#ifdef LacewingDebug

	if(Expression)
		return;

	#ifdef _MSC_VER
		__asm int 3;
	#else
		(*(int *) 0) = 1;
	#endif	
	
#endif
}

void LacewingInitialise();

#include "../include/Lacewing.h"

#ifdef LacewingWindows

    #define WINVER        0x0501
    #define _WIN32_WINNT  0x0501

    #define I64Format   "%I64d"
    #define UDFormat    "%d"

    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #include <mswsock.h>

    #define SECURITY_WIN32

    #include <security.h>
    #include <sspi.h>
    #include <wincrypt.h>
    #include <wintrust.h>
    #include <schannel.h>

    #include <process.h>
    #include <new.h>

    #include <ctime>

    #undef SendMessage
    #undef Yield

    #ifndef SO_CONNECT_TIME
        #define SO_CONNECT_TIME 0x700C
    #endif
    
    inline void LacewingCloseSocket(SOCKET Socket)
    {
        CancelIo((HANDLE) Socket);
        closesocket(Socket);
    }

    #define LacewingGetSocketError() WSAGetLastError()
    #define LacewingGetLastError() GetLastError()

    /* Safe providing gmtime() is never used outside this function */

    extern Lacewing::Sync Sync_GMTime;

    inline void gmtime_r(const time_t * T, tm * TM)
    {
        Lacewing::Sync::Lock Lock(Sync_GMTime);

        tm * _TM = gmtime(T);

        if(!_TM)
        {
            memset(TM, 0, sizeof(tm));
            return;
        }

        memcpy(TM, _TM, sizeof(tm));
    }

    inline time_t FileTimeToUnixTime(const FILETIME &FileTime)
    {
        LARGE_INTEGER LargeInteger;

        LargeInteger.LowPart = FileTime.dwLowDateTime;
        LargeInteger.HighPart = FileTime.dwHighDateTime;

        return (time_t)((LargeInteger.QuadPart - 116444736000000000) / 10000000);
    }

    #define LacewingYield()             Sleep(0)
    #define LacewingSocketError(E)      WSA##E
    #define lw_vsnprintf                _vsnprintf

#else

    #ifndef Lacewing64
        #define I64Format   "%lld"
    #else
        #define I64Format   "%ld"
    #endif
    
    #define UDFormat    "%ud"

    #include <stdio.h>
    #include <sys/types.h> 
    #include <sys/stat.h>
    #include <sys/socket.h>
    #include <sys/poll.h>
    #include <sys/utsname.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <pthread.h>
    #include <errno.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <sched.h>
    
    #ifdef HAVE_SYS_TIMERFD_H
        #include <sys/timerfd.h>
        
        #ifndef LacewingUseKQueue
            #define LacewingUseTimerFD
        #endif
    #endif
    
    #ifdef HAVE_SYS_SENDFILE_H
        #include <sys/sendfile.h>
    #endif

    #if HAVE_DECL_TCP_CORK
        #define LacewingCork TCP_CORK
    #else
        #if HAVE_DECL_TCP_NOPUSH
            #define LacewingCork TCP_NOPUSH 
        #else
            #error "No TCP_CORK or TCP_NOPUSH available on this platform"
        #endif
    #endif

    #ifdef HAVE_SYS_EPOLL_H

        #define LacewingUseEPoll
        #include <sys/epoll.h>

        #if !HAVE_DECL_EPOLLRDHUP
            #define EPOLLRDHUP 0x2000
        #endif

    #else

        #ifdef HAVE_KQUEUE
        
            #define LacewingUseKQueue
            #include <sys/event.h>

        #else
            #error Either EPoll or KQueue are required
        #endif
        
    #endif

    #include <string.h>

    #define stricmp strcasecmp

    #include <limits.h>
    #define MAX_PATH PATH_MAX

    #ifdef HAVE_SYS_PRCTL_H
        #include <sys/prctl.h>
    #endif

    #ifdef HAVE_CORESERVICES_CORESERVICES_H
        #include <CoreServices/CoreServices.h>
        #define LacewingUseMPSemaphore
    #else
        #include <semaphore.h>
    #endif

    #ifdef HAVE_OPENSSL_MD5_H
        #include <openssl/ssl.h>
        #include <openssl/md5.h>
        #include <openssl/err.h>
    #else
        #pragma error "OpenSSL not found. Install OpenSSL and run ./configure again."
    #endif

    inline void Sleep(int Milliseconds)
    {
        sleep(Milliseconds == -1 ? -1 : Milliseconds / 1000);
    }

    #define SOCKET int
    #define LacewingCloseSocket(S) close(S)
    #define LacewingGetSocketError() errno
    #define LacewingGetLastError() errno

    #define __int8  int8_t
    #define __int16 int16_t
    #define __int32 int32_t

    #define _atoi64 atoll

    typedef long LONG;

    inline void SetEnvironmentVariable(const char * Name, const char * Value)
    {
        setenv(Name, Value, 1);
    }

    #define LacewingYield()             sched_yield()
    #define LacewingSocketError(E)      E
    #define lw_vsnprintf                vsnprintf
    
#endif

inline void DisableNagling (SOCKET Socket)
{
    int Yes = 1;
    setsockopt(Socket, SOL_SOCKET, TCP_NODELAY, (char *) &Yes, sizeof(Yes));
}

#include <stdarg.h>

#ifdef HAVE_MALLOC_H
    #include <malloc.h>
#endif

inline int LacewingFormat(char *& Output, const char * Format, va_list args)
{
    #ifdef HAVE_DECL_VASPRINTF

        int Count = vasprintf(Output, Format, args);

        if(Count == -1)
            Output = 0;

        return Count;

    #else

        int Count = lw_vsnprintf(0, 0, Format, args); 

        char * Buffer = (char *) malloc(Count + 1);

        if(!Buffer)
        {
            Output = 0;
            return 0;
        }

        lw_vsnprintf(Buffer, Count + 1, Format, args);
        return Count;

    #endif
}

#include <fstream>
#include <vector>
#include <list>
#include <set>
#include <queue>
#include <map>
#include <deque>
#include <stack>
#include <sstream>

using namespace std;

#include <iostream>
using namespace std;

extern Lacewing::Sync Sync_DebugOutput;
extern ostringstream DebugOutput;

#ifdef LacewingDebug
    #ifdef COXSDK
        #define DebugOut(X) do { Lacewing::Sync::Lock Lock(Sync_DebugOutput); \
                                 DebugOutput << "Thread " << (int) Lacewing::CurrentThreadID() \
                                    << ": " << X << endl; OutputDebugString(DebugOutput.str().c_str()); DebugOutput.str(""); } while(0);
    #else
        #define DebugOut(X) do { Lacewing::Sync::Lock Lock(Sync_DebugOutput); \
                                 DebugOutput << "Thread " << (int) Lacewing::CurrentThreadID() \
                                    << ": " << X << endl; printf("%s", DebugOutput.str().c_str()); DebugOutput.str(""); } while(0);
    #endif
#else
    #define DebugOut(X)
#endif

inline void LacewingSyncIncrement(volatile long * Target)
{
    #ifdef __GNUC__
        __sync_add_and_fetch(Target, 1);
    #else
        #ifdef _MSC_VER
            InterlockedIncrement(Target);
        #else
            #error "Don't know how to implement LacewingSyncIncrement on this platform"
        #endif
    #endif
}

inline void LacewingSyncDecrement(volatile long * Target)
{
    #ifdef __GNUC__
        __sync_sub_and_fetch(Target, 1);
    #else
        #ifdef _MSC_VER
            InterlockedDecrement(Target);
        #else
            #error "Don't know how to implement LacewingSyncDecrement on this platform"
        #endif
    #endif
}

inline long LacewingSyncCompareExchange(volatile long * Target, long NewValue, long OldValue)
{
    #ifdef __GNUC__
        return __sync_val_compare_and_swap(Target, OldValue, NewValue);
    #else
        #ifdef _MSC_VER
            return InterlockedCompareExchange(Target, NewValue, OldValue);
        #else
            #error "Don't know how to implement LacewingSyncCompareExchange on this platform"
        #endif
    #endif
}

inline void LacewingSyncExchange(volatile long * Target, long NewValue)
{
    #ifdef __GNUC__
        
        for(;;)
        {
            long Current = *Target;

            if(__sync_val_compare_and_swap(Target, Current, NewValue) == Current)
                break;
        }

    #else
        #ifdef _MSC_VER
            InterlockedExchange(Target, NewValue);
        #else
            #error "Don't know how to implement LacewingSyncExchange on this platform"
        #endif
    #endif
}

#include "Looper.h"
#include "TimeHelper.h"
#include "ThreadTracker.h"
#include "MD5Hasher.h"
#include "ReceiveBuffer.h"
#include "MessageBuilder.h"
#include "MessageReader.h"
#include "Backlog.h"

#ifdef LacewingWindows
    #include "windows/EventPump.h"
#else
    #include "unix/EventPump.h"
#endif

#define AutoHandlerFunctions(Public, Internal, HandlerName)              \
    void Public::on##HandlerName(Public::Handler##HandlerName Handler)   \
    {   ((Internal *) InternalTag)->Handler##HandlerName = Handler;      \
    }                                                                    \

#define AutoHandlerFlat(real_class, flat, handler_upper, handler_lower) \
    void flat##_on##handler_lower (flat * _flat, flat##_handler_##handler_lower _handler) \
    {   ((real_class *) _flat)->on##handler_upper((real_class::Handler##handler_upper) _handler); \
    }

inline void GetSockaddr(Lacewing::Address &Address, sockaddr_in &out)
{
    memset(&out, 0, sizeof(sockaddr_in));

    out.sin_family      = AF_INET;
    out.sin_port        = htons((short) Address.Port());
    out.sin_addr.s_addr = Address.IP();
}

inline void Lowercase(char * s)
{
    for(; *s; ++ s)
        *s = (char) tolower(*s);
}

inline void Uppercase(char * s)
{
    for(; *s; ++ s)
        *s = (char) toupper(*s);
}

inline bool URLDecode(char * URL, char * New, unsigned int OutLength)
{
    size_t Length = strlen(URL);
    
    if((Length + 8) > OutLength)
        return false;

    char * out = New;
    char * in = (char *) URL;

    while(*in)
    {
        if((*in) == '%')
        {
            char last = in[3];
            in[3] = 0;

            sscanf(++ in, "%X", (unsigned int *) out);
            *((++ in) + 1) = last;
        }
        else
            *out = *in == '+' ? ' ' : *in;

        ++ out;

        if(!*(in ++))
            break;
    }

    *out = 0;
    return true;
}

inline const char * GetPathFileExtension(const char * Filename)
{
    const char * it = Filename + strlen(Filename);

    while(*it != '.')
    {
        if(it == Filename)
            return "";

        -- it;
    }

    return ++ it;
}

inline const char * GetPathFileName(const char * Filename)
{
    const char * it = Filename + strlen(Filename);

    while(*it != '\\' && *it != '/' && it != Filename)
        -- it;

    if(it != Filename && it[1])
        ++ it;

    return it;
}

inline bool BeginsWith(const char * String, const char * Substring)
{
    while(*Substring)
    {
        if(!*String || tolower(*String) != tolower(*Substring))
            return false;

        ++ Substring;
        ++ String;
    }

    return true;
}

inline void Trim(char * Input, char *& Output)
{
    Output = Input;

    while(isspace(*Output))
        ++ Output;

    if(!*Output)
        return;

    char * End = Output + strlen(Output) - 1;

    while(isspace(*End))
        *(End --) = 0;
}

