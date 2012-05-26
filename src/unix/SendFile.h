
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef __linux__
    #define LacewingLinuxSendfile
#else
    #ifdef __APPLE__
        #define LacewingMacSendFile
    #else
        #ifdef __FreeBSD__
            #define LacewingFreeBSDSendFile
        #else
            #warning "Don't know how to use sendfile(2) on this platform."
        #endif
    #endif
#endif

/* FDStream makes the assumption that this will fail for anything but a regular
 * file (i.e. something that is always considered read ready)
 */

inline lw_i64 LacewingSendFile (int from, int dest, lw_i64 size)
{
    #ifdef LacewingLinuxSendfile
        
        int sent = sendfile (dest, from, 0, size);

        if (sent == -1)
        {
            if (errno == EAGAIN)
                return 0;

            return -1;
        }

        return sent;

    #endif

    #ifdef LacewingFreeBSDSendFile

        off_t sent;

        if (sendfile (from, dest, lseek (from, 0, SEEK_CUR), size, 0, &sent, 0) != 0)
        {
            if(errno != EAGAIN)
                return -1;
        }

        lseek (from, sent, SEEK_CUR);
                                    
        return sent;

    #endif

    #ifdef LacewingMacSendFile

        off_t bytes = size;

        if (sendfile (from, dest, lseek (from, 0, SEEK_CUR), &bytes, 0, 0) != 0)
        {
            if (errno != EAGAIN)
                return -1;
        }
                  
        lseek (from, bytes, SEEK_CUR);
        
        return bytes;
    
    #endif

    errno = EINVAL;
    return -1;
}


